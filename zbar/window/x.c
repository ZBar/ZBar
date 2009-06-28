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

extern int _zbar_window_probe_ximage(zbar_window_t*);
extern int _zbar_window_probe_xshm(zbar_window_t*);
extern int _zbar_window_probe_xv(zbar_window_t*);

static inline unsigned long window_alloc_color (zbar_window_t *w,
                                                Colormap cmap,
                                                unsigned short r,
                                                unsigned short g,
                                                unsigned short b)
{
    XColor color;
    color.red = r;
    color.green = g;
    color.blue = b;
    color.flags = 0;
    XAllocColor(w->display, cmap, &color);
    return(color.pixel);
}

static inline int window_alloc_colors (zbar_window_t *w)
{
    Colormap cmap = DefaultColormap(w->display, DefaultScreen(w->display));
    int i;
    for(i = 0; i < 8; i++)
        w->colors[i] =
            window_alloc_color(w, cmap,
                               (i & 4) ? (0xcc * 0x101) : 0,
                               (i & 2) ? (0xcc * 0x101) : 0,
                               (i & 1) ? (0xcc * 0x101) : 0);

    w->logo_colors[0] = window_alloc_color(w, cmap, 0xd709, 0x3333, 0x3333);
    w->logo_colors[1] = window_alloc_color(w, cmap, 0xa3d6, 0x0000, 0x0000);
    return(0);
}

static inline int window_hide_cursor (zbar_window_t *w)
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

int _zbar_window_resize (zbar_window_t *w)
{
    int lbw;
    if(w->height * 8 / 10 <= w->width)
        lbw = w->height / 36;
    else
        lbw = w->width * 5 / 144;
    if(lbw < 1)
        lbw = 1;
    w->logo_scale = lbw;
    if(w->logo_zbars)
        XDestroyRegion(w->logo_zbars);
    w->logo_zbars = XCreateRegion();

    int x0 = w->width / 2;
    int y0 = w->height / 2;
    int by0 = y0 - 54 * lbw / 5;
    int bh = 108 * lbw / 5;

    static const int bx[5] = { -6, -3, -1,  2,  5 };
    static const int bw[5] = {  1,  1,  2,  2,  1 };

    int i;
    for(i = 0; i < 5; i++) {
        XRectangle *bar = &w->logo_bars[i];
        bar->x = x0 + lbw * bx[i];
        bar->y = by0;
        bar->width = lbw * bw[i];
        bar->height = bh;
        XUnionRectWithRegion(bar, w->logo_zbars, w->logo_zbars);
    }

    static const int zx[4] = { -7,  7, -7,  7 };
    static const int zy[4] = { -8, -8,  8,  8 };

    for(i = 0; i < 4; i++) {
        w->logo_z[i].x = x0 + lbw * zx[i];
        w->logo_z[i].y = y0 + lbw * zy[i];
    }
    return(0);
}

int _zbar_window_attach (zbar_window_t *w,
                         void *display,
                         unsigned long win)
{
    if(w->display) {
        /* cleanup existing resources */
        if(w->gc)
            XFreeGC(w->display, w->gc);
        assert(!w->exposed);
        if(w->logo_zbars) {
            XDestroyRegion(w->logo_zbars);
            w->logo_zbars = NULL;
        }
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
    _zbar_window_resize(w);

    window_alloc_colors(w);
    window_hide_cursor(w);

    /* FIXME add interface preference override */
#ifdef HAVE_X11_EXTENSIONS_XVLIB_H
    if(!_zbar_window_probe_xv(w))
        return(0);
#endif

    zprintf(1, "falling back to XImage\n");
    return(_zbar_window_probe_ximage(w));
}

int _zbar_window_clear (zbar_window_t *w)
{
    if(!w->display)
        return(0);
    int screen = DefaultScreen(w->display);
    XSetForeground(w->display, w->gc, WhitePixel(w->display, screen));
    XFillRectangle(w->display, w->xwin, w->gc, 0, 0, w->width, w->height);
    return(0);
}

int _zbar_window_draw_marker(zbar_window_t *w,
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

int _zbar_window_draw_logo (zbar_window_t *w)
{
    if(w->exposed)
        XSetRegion(w->display, w->gc, w->exposed);

    int screen = DefaultScreen(w->display);

    /* clear to white */
    XSetForeground(w->display, w->gc, WhitePixel(w->display, screen));
    XFillRectangle(w->display, w->xwin, w->gc, 0, 0, w->width, w->height);

    if(!w->logo_scale || !w->logo_zbars)
        return(0);

    XSetForeground(w->display, w->gc, BlackPixel(w->display, screen));
    XFillRectangles(w->display, w->xwin, w->gc, w->logo_bars, 5);

    XSetLineAttributes(w->display, w->gc, 2 * w->logo_scale,
                       LineSolid, CapRound, JoinRound);

    XSetForeground(w->display, w->gc, w->logo_colors[0]);
    XDrawLines(w->display, w->xwin, w->gc, w->logo_z, 4, CoordModeOrigin);

    if(w->exposed) {
        XIntersectRegion(w->logo_zbars, w->exposed, w->exposed);
        XSetRegion(w->display, w->gc, w->exposed);
    }
    else
        XSetRegion(w->display, w->gc, w->logo_zbars);

    XSetForeground(w->display, w->gc, w->logo_colors[1]);
    XDrawLines(w->display, w->xwin, w->gc, w->logo_z, 4, CoordModeOrigin);
    XFlush(w->display);

    /* reset GC */
    XSetLineAttributes(w->display, w->gc, 0,
                       LineSolid, CapButt, JoinMiter);
    XSetClipMask(w->display, w->gc, None);
    return(0);
}
