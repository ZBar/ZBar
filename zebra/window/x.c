/*------------------------------------------------------------------------
 *  Copyright 2007-2008 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the Zebra Barcode Library.
 *
 *  The Zebra Barcode Library is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The Zebra Barcode Library is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the Zebra Barcode Library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zebra
 *------------------------------------------------------------------------*/

#include "window.h"
#include "processor.h"
#include <X11/keysym.h>

static inline int window_alloc_colors (zebra_window_t *w)
{
    Colormap cmap = DefaultColormap(w->display, DefaultScreen(w->display));
    XColor color;
    int i;
    for(i = 0; i < 8; i++) {
        color.red   = (i & 4) ? (0xcc * 0x101) : 0;
        color.green = (i & 2) ? (0xcc * 0x101) : 0;
        color.blue  = (i & 1) ? (0xcc * 0x101) : 0;
        color.flags = 0;
        XAllocColor(w->display, cmap, &color);
        w->colors[i] = color.pixel;
    }
    return(0);
}

static inline int window_hide_cursor (zebra_window_t *w)
{
    /* FIXME this seems lame...there must be a better way */
    Pixmap empty = XCreatePixmap(w->display, w->xwin, 1, 1, 1);
    GC gc = XCreateGC(w->display, empty, 0, NULL);
    XDrawPoint(w->display, empty, gc, 0, 0);
    XColor black;
    memset(&black, 0, sizeof(black));
    int screen = DefaultScreen(w->display);
    black.pixel = BlackPixel(w->display, screen);
    Cursor cursor =
        XCreatePixmapCursor(w->display, empty, empty, &black, &black, 0, 0);
    XDefineCursor(w->display, w->xwin, cursor);
    XFreeCursor(w->display, cursor);
    XFreeGC(w->display, gc);
    XFreePixmap(w->display, empty);
    return(0);
}

int _zebra_window_attach (zebra_window_t *w,
                          void *display,
                          unsigned long win)
{
    if(w->display) {
        /* cleanup existing resources */
        if(w->gc)
            XFreeGC(w->display, w->gc);
        w->display = NULL;
    }
    w->xwin = 0;

    if(!display || !win)
        return(0);

    w->display = display;
    w->xwin = win;
    w->gc = XCreateGC(display, win, 0, NULL);

    XWindowAttributes attr;
    XGetWindowAttributes(w->display, w->xwin, &attr);
    w->width = attr.width;
    w->height = attr.height;

    window_alloc_colors(w);
    window_hide_cursor(w);

    /* FIXME add interface preference override */
#ifdef HAVE_X11_EXTENSIONS_XVLIB_H
    if(!_zebra_window_probe_xv(w))
        return(0);
#endif

    zprintf(1, "falling back to XImage\n");
    return(_zebra_window_probe_ximage(w));
}

static int x_handle_event (zebra_processor_t *proc)
{
    XEvent ev;
    XNextEvent(proc->display, &ev);

    switch(ev.type) {
    case Expose:
        /* FIXME ignore when running */
        if(!ev.xexpose.count)
            zebra_window_redraw(proc->window);
        break;

    case ConfigureNotify:
        zprintf(3, "resized to %d x %d\n",
                ev.xconfigure.width, ev.xconfigure.height);
        zebra_window_resize(proc->window,
                            ev.xconfigure.width, ev.xconfigure.height);
        break;

    case ClientMessage:
        if((ev.xclient.message_type ==
            XInternAtom(proc->display, "WM_PROTOCOLS", 0)) &&
           ev.xclient.format == 32 &&
           (ev.xclient.data.l[0] ==
            XInternAtom(proc->display, "WM_DELETE_WINDOW", 0))) {
            zprintf(3, "WM_DELETE_WINDOW\n");
            _zebra_window_set_visible(proc, 0);
            return(err_capture(proc, SEV_WARNING, ZEBRA_ERR_CLOSED, __func__,
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

    default:
        /* ignored */;
    }
    return(0);
}

int _zebra_window_handle_events (zebra_processor_t *proc,
                                 int block)
{
    int rc = 0;
    while(!rc && (block || XPending(proc->display))) {
        proc->input = x_handle_event(proc);
        rc = proc->input;
    }

    switch(rc) {
    case 'q':
        _zebra_window_set_visible(proc, 0);
        rc = err_capture(proc, SEV_WARNING, ZEBRA_ERR_CLOSED, __func__,
                         "user closed display window");
        break;

    case 'd': {
        /* FIXME localtime not threadsafe */
        /* FIXME need ms resolution */
        /*struct tm *t = localtime(time(NULL));*/
        zebra_image_write(proc->window->image, "zebra");
        break;
    }
    }

    if(rc)
        emit(proc, EVENT_INPUT);
    return(rc);
}

static int x_connection_handler (zebra_processor_t *proc,
                                 int i)
{
    return(_zebra_window_handle_events(proc, 0));
}

static int x_internal_handler (zebra_processor_t *proc,
                               int i)
{
    XProcessInternalConnection(proc->display, proc->polling.fds[i].fd);
    return(_zebra_window_handle_events(proc, 0));
}

static void x_internal_watcher (Display *display,
                                XPointer client_arg,
                                int fd,
                                Bool opening,
                                XPointer *watch_arg)
{
    zebra_processor_t *proc = (void*)client_arg;
    if(opening)
        add_poll(proc, fd, x_internal_handler);
    else
        remove_poll(proc, fd);
}

int _zebra_window_open (zebra_processor_t *proc,
                        char *title,
                        unsigned width,
                        unsigned height)
{
    proc->display = XOpenDisplay(NULL);
    if(!proc->display)
        return(err_capture_str(proc, SEV_ERROR, ZEBRA_ERR_XDISPLAY, __func__,
                               "unable to open X display",
                               XDisplayName(NULL)));

    add_poll(proc, ConnectionNumber(proc->display), x_connection_handler);
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
        return(err_capture(proc, SEV_ERROR, ZEBRA_ERR_XPROTO, __func__,
                           "creating window"));
    }

    XStoreName(proc->display, proc->xwin, title);

    XClassHint *class_hint = XAllocClassHint();
    class_hint->res_name = "zebra";
    class_hint->res_class = "zebra";
    XSetClassHint(proc->display, proc->xwin, class_hint);
    XFree(class_hint);
    class_hint = NULL;

    Atom wm_delete_window = XInternAtom(proc->display, "WM_DELETE_WINDOW", 0);
    if(wm_delete_window)
        XSetWMProtocols(proc->display, proc->xwin, &wm_delete_window, 1);

    return(0);
}

int _zebra_window_close (zebra_processor_t *proc)
{
    if(proc->display) {
        if(proc->xwin) {
            XDestroyWindow(proc->display, proc->xwin);
            proc->xwin = 0;
        }
        remove_poll(proc, ConnectionNumber(proc->display));
        XCloseDisplay(proc->display);
        proc->display = NULL;
    }
    return(0);
}

int _zebra_window_resize (zebra_processor_t *proc,
                          unsigned width,
                          unsigned height)
{
    if(proc->display) {
        XResizeWindow(proc->display, proc->xwin, width, height);
        _zebra_window_clear(proc->window);
        XFlush(proc->display);
    }
    return(0);
}

int _zebra_window_set_visible (zebra_processor_t *proc,
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

int _zebra_window_invalidate (zebra_window_t *w)
{
    if(!w->display)
        return(0);
    XClearArea(w->display, w->xwin, 0, 0, w->width, w->height, 1);
    XFlush(w->display);
    return(0);
}

int _zebra_window_clear (zebra_window_t *w)
{
    if(!w->display)
        return(0);
    int screen = DefaultScreen(w->display);
    XSetForeground(w->display, w->gc, BlackPixel(w->display, screen));
    XFillRectangle(w->display, w->xwin, w->gc, 0, 0, w->width, w->height);
    return(0);
}

int _zebra_window_draw_marker(zebra_window_t *w,
                              uint32_t rgb,
                              const point_t *p)
{
    XSetForeground(w->display, w->gc, w->colors[rgb]);

    int x = p->x;
    if(x < 3)
        x = 3;
    else if(x > w->width - 4)
        x = w->width - 4;

    int y = p->y;
    if(y < 3)
        y = 3;
    else if(y > w->height - 4)
        y = w->height - 4;

    XDrawRectangle(w->display, w->xwin, w->gc, x - 2, y - 2, 4, 4);
    XDrawLine(w->display, w->xwin, w->gc, x, y - 3, x, y + 3);
    XDrawLine(w->display, w->xwin, w->gc, x - 3, y, x + 3, y);
    return(0);
}
