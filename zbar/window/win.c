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
#include "image.h"
#include "win.h"

int _zbar_window_vfw_init(zbar_window_t *w);
int _zbar_window_dib_init(zbar_window_t *w);

int _zbar_window_draw_polygon (zbar_window_t *w,
                               uint32_t rgb,
                               const point_t *pts,
                               int npts)
{
    /* FIXME TBD */
    return(-1);
}

int _zbar_window_draw_marker(zbar_window_t *w,
                             uint32_t rgb,
                             const point_t *p)
{
    /* FIXME TBD */
    return(-1);
}

int _zbar_window_resize (zbar_window_t *w)
{
    window_state_t *win = w->state;
    int lbw;
    if(w->height * 8 / 10 <= w->width)
        lbw = w->height / 36;
    else
        lbw = w->width * 5 / 144;
    if(lbw < 1)
        lbw = 1;
    win->logo_scale = lbw;
    zprintf(7, "%dx%d scale=%d\n", w->width, w->height, lbw);
    if(win->logo_zbars) {
        DeleteObject(win->logo_zbars);
        win->logo_zbars = NULL;
    }
    if(win->logo_zpen)
        DeleteObject(win->logo_zpen);
    if(win->logo_zbpen)
        DeleteObject(win->logo_zbpen);

    LOGBRUSH lb = { 0, };
    lb.lbStyle = BS_SOLID;
    lb.lbColor = RGB(0xd7, 0x33, 0x33);
    win->logo_zpen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID |
                                  PS_ENDCAP_ROUND | PS_JOIN_ROUND,
                                  lbw * 2, &lb, 0, NULL);

    lb.lbColor = RGB(0xa4, 0x00, 0x00);
    win->logo_zbpen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID |
                                   PS_ENDCAP_ROUND | PS_JOIN_ROUND,
                                   lbw * 2, &lb, 0, NULL);

    int x0 = w->width / 2;
    int y0 = w->height / 2;
    int by0 = y0 - 54 * lbw / 5;
    int bh = 108 * lbw / 5;

    static const int bx[5] = { -6, -3, -1,  2,  5 };
    static const int bw[5] = {  1,  1,  2,  2,  1 };

    int i;
    for(i = 0; i < 5; i++) {
        int x = x0 + lbw * bx[i];
        HRGN bar = CreateRectRgn(x, by0,
                                 x + lbw * bw[i], by0 + bh);
        if(win->logo_zbars) {
            CombineRgn(win->logo_zbars, win->logo_zbars, bar, RGN_OR);
            DeleteObject(bar);
        }
        else
            win->logo_zbars = bar;
    }

    static const int zx[4] = { -7,  7, -7,  7 };
    static const int zy[4] = { -8, -8,  8,  8 };

    for(i = 0; i < 4; i++) {
        win->logo_z[i].x = x0 + lbw * zx[i];
        win->logo_z[i].y = y0 + lbw * zy[i];
    }
    return(0);
}

int _zbar_window_attach (zbar_window_t *w,
                         void *display,
                         unsigned long unused)
{
    window_state_t *win = w->state;
    if(w->display) {
        /* FIXME cleanup existing resources */
        w->display = NULL;
    }

    if(!display) {
        if(win) {
            free(win);
            w->state = NULL;
        }
        return(0);
    }

    if(!win)
        win = w->state = calloc(1, sizeof(window_state_t));

    w->display = display;

    win->bih.biSize = sizeof(win->bih);
    win->bih.biPlanes = 1;

    HDC hdc = GetDC(w->display);
    if(!hdc)
        return(-1/*FIXME*/);
    win->bih.biXPelsPerMeter =
        1000L * GetDeviceCaps(hdc, HORZRES) / GetDeviceCaps(hdc, HORZSIZE);
    win->bih.biYPelsPerMeter =
        1000L * GetDeviceCaps(hdc, VERTRES) / GetDeviceCaps(hdc, VERTSIZE);
    ReleaseDC(w->display, hdc);

    return(_zbar_window_dib_init(w));
}

int _zbar_window_clear (zbar_window_t *w)
{
    HDC hdc = GetDC(w->display);
    if(!hdc)
        return(-1/*FIXME*/);

    RECT r = { 0, 0, w->width, w->height };
    FillRect(hdc, &r, GetStockObject(BLACK_BRUSH));

    ReleaseDC(w->display, hdc);
    ValidateRect(w->display, NULL);
    return(0);
}

int _zbar_window_flush (zbar_window_t *w)
{
    return(0);
}

int _zbar_window_draw_logo (zbar_window_t *w)
{
    if(!w->display)
        return(-1);

    HDC hdc = GetDC(w->display);
    if(!hdc || !SaveDC(hdc))
        return(-1/*FIXME*/);

    window_state_t *win = w->state;

    /* FIXME buffer offscreen */
    HRGN rgn = CreateRectRgn(0, 0, w->width, w->height);
    CombineRgn(rgn, rgn, win->logo_zbars, RGN_DIFF);
    FillRgn(hdc, rgn, GetStockObject(WHITE_BRUSH));
    DeleteObject(rgn);

    FillRgn(hdc, win->logo_zbars, GetStockObject(BLACK_BRUSH));

    SelectObject(hdc, win->logo_zpen);
    Polyline(hdc, win->logo_z, 4);

    ExtSelectClipRgn(hdc, win->logo_zbars, RGN_AND);
    SelectObject(hdc, win->logo_zbpen);
    Polyline(hdc, win->logo_z, 4);

    RestoreDC(hdc, -1);
    ReleaseDC(w->display, hdc);
    ValidateRect(w->display, NULL);
    return(0);
}

int _zbar_window_bih_init (zbar_window_t *w,
                           zbar_image_t *img)
{
    window_state_t *win = w->state;
    switch(w->format) {
    case fourcc('J','P','E','G'): {
        win->bih.biBitCount = 0;
        win->bih.biCompression = BI_JPEG;
        break;
    }
    case fourcc('B','G','R','3'): {
        win->bih.biBitCount = 24;
        win->bih.biCompression = BI_RGB;
        break;
    }
    case fourcc('B','G','R','4'): {
        win->bih.biBitCount = 32;
        win->bih.biCompression = BI_RGB;
        break;
    }
    default:
        assert(0);
        /* FIXME PNG? */
    }
    win->bih.biSizeImage = img->datalen;

    zprintf(20, "biCompression=%d biBitCount=%d\n",
            (int)win->bih.biCompression, win->bih.biBitCount);

    return(0);
}
