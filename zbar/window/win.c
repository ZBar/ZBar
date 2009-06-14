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

int _zbar_window_vfw_init(zbar_window_t *w);

int
_zbar_window_draw_marker(zbar_window_t *w,
                         uint32_t rgb,
                         const point_t *p)
{
    return(-1);
}

int
_zbar_window_resize (zbar_window_t *w)
{
    int lbw;
    if(w->height * 8 / 10 <= w->width)
        lbw = w->height / 36;
    else
        lbw = w->width * 5 / 144;
    if(lbw < 1)
        lbw = 1;
    w->logo_scale = lbw;
    zprintf(7, "%dx%d scale=%d\n", w->width, w->height, lbw);
    if(w->logo_zbars) {
        DeleteObject(w->logo_zbars);
        w->logo_zbars = NULL;
    }
    if(w->logo_zpen)
        DeleteObject(w->logo_zpen);
    if(w->logo_zbpen)
        DeleteObject(w->logo_zbpen);

    LOGBRUSH lb = { 0, };
    lb.lbStyle = BS_SOLID;
    lb.lbColor = RGB(0xd7, 0x33, 0x33);
    w->logo_zpen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID |
                                PS_ENDCAP_ROUND | PS_JOIN_ROUND,
                                lbw * 2, &lb, 0, NULL);

    lb.lbColor = RGB(0xa4, 0x00, 0x00);
    w->logo_zbpen = ExtCreatePen(PS_GEOMETRIC | PS_SOLID |
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
        if(w->logo_zbars) {
            CombineRgn(w->logo_zbars, w->logo_zbars, bar, RGN_OR);
            DeleteObject(bar);
        }
        else
            w->logo_zbars = bar;
    }

    static const int zx[4] = { -7,  7, -7,  7 };
    static const int zy[4] = { -8, -8,  8,  8 };

    for(i = 0; i < 4; i++) {
        w->logo_z[i].x = x0 + lbw * zx[i];
        w->logo_z[i].y = y0 + lbw * zy[i];
    }
    return(0);
}

int
_zbar_window_attach (zbar_window_t *w,
                     void *display,
                     unsigned long win)
{
    if(w->hwnd) {
        /* FIXME cleanup existing resources */
        w->hwnd = NULL;
    }

    if(!display)
        return(0);

    w->hwnd = display;

    return(_zbar_window_vfw_init(w));
}

int
_zbar_window_clear (zbar_window_t *w)
{
    HDC hdc = GetDC(w->hwnd);
    if(!hdc)
        return(-1/*FIXME*/);

    RECT r = { 0, 0, w->width, w->height };
    FillRect(hdc, &r, GetStockObject(BLACK_BRUSH));

    ReleaseDC(w->hwnd, hdc);
    ValidateRect(w->hwnd, NULL);
    return(0);
}

int
_zbar_window_draw_logo (zbar_window_t *w)
{
    if(!w->hwnd)
        return(-1);

    HDC hdc = GetDC(w->hwnd);
    if(!hdc || !SaveDC(hdc))
        return(-1/*FIXME*/);

    /* FIXME buffer offscreen */
    HRGN rgn = CreateRectRgn(0, 0, w->width, w->height);
    CombineRgn(rgn, rgn, w->logo_zbars, RGN_DIFF);
    FillRgn(hdc, rgn, GetStockObject(WHITE_BRUSH));
    DeleteObject(rgn);

    FillRgn(hdc, w->logo_zbars, GetStockObject(BLACK_BRUSH));

    SelectObject(hdc, w->logo_zpen);
    Polyline(hdc, w->logo_z, 4);

    ExtSelectClipRgn(hdc, w->logo_zbars, RGN_AND);
    SelectObject(hdc, w->logo_zbpen);
    Polyline(hdc, w->logo_z, 4);

    RestoreDC(hdc, -1);
    ReleaseDC(w->hwnd, hdc);
    ValidateRect(w->hwnd, NULL);
    return(0);
}
