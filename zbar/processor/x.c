/*------------------------------------------------------------------------
 *  Copyright 2007-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/

#include "window.h"
#include "processor.h"
#include "posix.h"
#include <X11/keysym.h>

static int x_handle_event (zbar_processor_t *proc)
{
    XEvent ev;
    XNextEvent(proc->display, &ev);

    switch(ev.type) {
    case Expose: {
        /* FIXME ignore when running(?) */
        XExposeEvent *exp = (XExposeEvent*)&ev;
        XRectangle r;
        Region exposed = XCreateRegion();
        while(1) {
            assert(ev.type == Expose);
            r.x = exp->x;
            r.y = exp->y;
            r.width = exp->width;
            r.height = exp->height;
            XUnionRectWithRegion(&r, exposed, exposed);

            if(!exp->count)
                break;
            XNextEvent(proc->display, &ev);
        }

        proc->window->exposed = exposed;
        zbar_window_redraw(proc->window);
        proc->window->exposed = 0;

        XDestroyRegion(exposed);
        break;
    }

    case ConfigureNotify:
        zprintf(3, "resized to %d x %d\n",
                ev.xconfigure.width, ev.xconfigure.height);
        zbar_window_resize(proc->window,
                           ev.xconfigure.width, ev.xconfigure.height);
        _zbar_processor_invalidate(proc);
        break;

    case ClientMessage:
        if((ev.xclient.message_type ==
            XInternAtom(proc->display, "WM_PROTOCOLS", 0)) &&
           ev.xclient.format == 32 &&
           (ev.xclient.data.l[0] ==
            XInternAtom(proc->display, "WM_DELETE_WINDOW", 0))) {
            zprintf(3, "WM_DELETE_WINDOW\n");
            _zbar_processor_set_visible(proc, 0);
            return(err_capture(proc, SEV_WARNING, ZBAR_ERR_CLOSED, __func__,
                               "user closed display window"));
        }

    case KeyPress: {
        KeySym key = XLookupKeysym(&ev.xkey, 0);
        if((key & 0xff00) == 0xff00)
            key &= 0x00ff;
        /* FIXME this doesn't really work... */
        return(key & 0xffff);
    }
    case ButtonPress:
        switch(ev.xbutton.button) {
        case Button2: return(2);
        case Button3: return(3);
        case Button4: return(4);
        case Button5: return(5);
        }
        return(1);

    case DestroyNotify:
        zbar_window_attach(proc->window, NULL, 0);
        proc->xwin = 0;
        return(0);

    default:
        /* ignored */;
    }
    return(0);
}

int _zbar_processor_handle_events (zbar_processor_t *proc,
                                   int block)
{
    int rc = 0;
    while(!rc && (block || XPending(proc->display))) {
        proc->input = x_handle_event(proc);
        rc = proc->input;
    }

    switch(rc) {
    case 'q':
        _zbar_processor_set_visible(proc, 0);
        rc = err_capture(proc, SEV_WARNING, ZBAR_ERR_CLOSED, __func__,
                         "user closed display window");
        break;

    case 'd': {
        /* FIXME localtime not threadsafe */
        /* FIXME need ms resolution */
        /*struct tm *t = localtime(time(NULL));*/
        zbar_image_write(proc->window->image, "zbar");
        break;
    }
    }

    if(rc)
        _zbar_processor_notify(proc, EVENT_INPUT);
    return(rc);
}

static int x_connection_handler (zbar_processor_t *proc,
                                 int i)
{
    return(_zbar_processor_handle_events(proc, 0));
}

static int x_internal_handler (zbar_processor_t *proc,
                               int i)
{
    XProcessInternalConnection(proc->display, proc->state->polling.fds[i].fd);
    return(_zbar_processor_handle_events(proc, 0));
}

static void x_internal_watcher (Display *display,
                                XPointer client_arg,
                                int fd,
                                Bool opening,
                                XPointer *watch_arg)
{
    zbar_processor_t *proc = (void*)client_arg;
    if(opening)
        add_poll(proc->state, fd, x_internal_handler);
    else
        remove_poll(proc->state, fd);
}

int _zbar_processor_open (zbar_processor_t *proc,
                          char *title,
                          unsigned width,
                          unsigned height)
{
    proc->display = XOpenDisplay(NULL);
    if(!proc->display)
        return(err_capture_str(proc, SEV_ERROR, ZBAR_ERR_XDISPLAY, __func__,
                               "unable to open X display",
                               XDisplayName(NULL)));

    add_poll(proc->state, ConnectionNumber(proc->display),
             x_connection_handler);
    XAddConnectionWatch(proc->display, x_internal_watcher, (void*)proc);

    int screen = DefaultScreen(proc->display);
    XSetWindowAttributes attr;
    attr.event_mask = (ExposureMask | StructureNotifyMask |
                       KeyPressMask | ButtonPressMask);

    proc->xwin = XCreateWindow(proc->display,
                               RootWindow(proc->display, screen),
                               0, 0, width, height, 0,
                               CopyFromParent, InputOutput,
                               CopyFromParent, CWEventMask, &attr);
    if(!proc->xwin) {
        XCloseDisplay(proc->display);
        return(err_capture(proc, SEV_ERROR, ZBAR_ERR_XPROTO, __func__,
                           "creating window"));
    }

    XStoreName(proc->display, proc->xwin, title);

    XClassHint *class_hint = XAllocClassHint();
    class_hint->res_name = "zbar";
    class_hint->res_class = "zbar";
    XSetClassHint(proc->display, proc->xwin, class_hint);
    XFree(class_hint);
    class_hint = NULL;

    Atom wm_delete_window = XInternAtom(proc->display, "WM_DELETE_WINDOW", 0);
    if(wm_delete_window)
        XSetWMProtocols(proc->display, proc->xwin, &wm_delete_window, 1);

    if(zbar_window_attach(proc->window, proc->display, proc->xwin))
        return(err_copy(proc, proc->window));
    return(0);
}

int _zbar_processor_close (zbar_processor_t *proc)
{
    if(proc->window)
        zbar_window_attach(proc->window, NULL, 0);

    if(proc->display) {
        if(proc->xwin) {
            XDestroyWindow(proc->display, proc->xwin);
            proc->xwin = 0;
        }
        remove_poll(proc->state, ConnectionNumber(proc->display));
        XCloseDisplay(proc->display);
        proc->display = NULL;
    }
    return(0);
}

int _zbar_processor_invalidate (zbar_processor_t *proc)
{
    if(!proc->display || !proc->xwin)
        return(0);
    XClearArea(proc->display, proc->xwin, 0, 0,
               proc->window->width, proc->window->height, 1);
    XFlush(proc->display);
    return(0);
}

int _zbar_processor_set_size (zbar_processor_t *proc,
                              unsigned width,
                              unsigned height)
{
    if(proc->display || !proc->xwin) {
        XResizeWindow(proc->display, proc->xwin, width, height);
        XFlush(proc->display);
    }
    return(0);
}

int _zbar_processor_set_visible (zbar_processor_t *proc,
                                 int visible)
{
    if(visible)
        XMapRaised(proc->display, proc->xwin);
    else
        XUnmapWindow(proc->display, proc->xwin);
    XFlush(proc->display);
    proc->visible = visible != 0;
    return(0);
}
