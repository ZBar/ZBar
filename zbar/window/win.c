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

int _zbar_window_vfw_init(zbar_window_t *w);

static LRESULT CALLBACK
proc_handle_event (HWND hwnd,
                   UINT message,
                   WPARAM wparam,
                   LPARAM lparam)
{
    zbar_processor_t *proc =
        (zbar_processor_t*)GetWindowLongPtr(hwnd, GWL_USERDATA);

    switch(message) {

    case WM_NCCREATE:
        proc = ((LPCREATESTRUCT)lparam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)proc);
        proc->display = hwnd;
        break;

    case WM_SIZE: {
        RECT r;
        GetClientRect(hwnd, &r);
        assert(proc);
        zbar_window_resize(proc->window, r.right, r.bottom);
        return(0);
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        zbar_window_redraw(proc->window);
        EndPaint(hwnd, &ps);
        return(0);
    }

    case WM_CHAR: {
        proc->input = wparam;
        return(0);
    }

    case WM_LBUTTONDOWN: {
        proc->input = 1;
        return(0);
    }

    case WM_MBUTTONDOWN: {
        proc->input = 2;
        return(0);
    }

    case WM_RBUTTONDOWN: {
        proc->input = 3;
        return(0);
    }

    case WM_DESTROY:
        zprintf(1, "DESTROY\n");
        proc->display = NULL;
        _zbar_window_attach(proc->window, NULL, 0);
        PostQuitMessage(0);
        return(0);
    }
    return(DefWindowProc(hwnd, message, wparam, lparam));
}

static inline ATOM
proc_register_class (HINSTANCE hmod)
{
    BYTE and_mask[1] = { 0xff };
    BYTE xor_mask[1] = { 0x00 };

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), 0, };
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hInstance = hmod;
    wc.lpfnWndProc = proc_handle_event;
    wc.lpszClassName = "_ZBar Class";
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.hCursor = CreateCursor(hmod, 0, 0, 1, 1, and_mask, xor_mask);

    return(RegisterClassEx(&wc));
}

int
_zbar_window_open (zbar_processor_t *proc,
                   char *title,
                   unsigned width,
                   unsigned height)
{
    HMODULE hmod = NULL;
    if(!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (void*)_zbar_window_open, (HINSTANCE*)&hmod)) {
        proc->err.errnum = GetLastError();
        return(err_capture(proc, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "failed to obtain module handle"));
    }
    zprintf(1, "hmod=%p\n", hmod);

    ATOM wca = proc_register_class(hmod);
    zprintf(1, "wca=%x\n", wca);
    if(!wca) {
        proc->err.errnum = GetLastError();
        return(err_capture(proc, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "failed to register window class"));
    }

    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = (WS_EX_APPWINDOW |
                     WS_EX_OVERLAPPEDWINDOW /*|
                     WS_EX_ACCEPTFILES*/);

    proc->display = CreateWindowEx(exstyle, (LPCTSTR)(long)wca, "ZBar", style,
                                   CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                                   NULL, NULL, hmod, proc);
    if(!proc->display) {
        proc->err.errnum = GetLastError();
        return(err_capture(proc, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "failed to open window"));
    }

    return(0);
}

int _zbar_window_close (zbar_processor_t *proc)
{
    if(proc->display) {
        DestroyWindow(proc->display);
        proc->display = NULL;
    }
    return(0);
}

int
_zbar_window_set_visible (zbar_processor_t *proc,
                          int visible)
{
    zprintf(1, "\n");
    ShowWindow(proc->display, (visible) ? SW_SHOWNORMAL : SW_HIDE);
    if(visible)
        UpdateWindow(proc->display);
    proc->visible = (visible != 0);
    /* no error conditions */
    return(0);
}

int _zbar_window_invalidate (zbar_window_t *w)
{
    if(!InvalidateRect(w->hwnd, NULL, 0))
        return(-1/*FIXME*/);

    return(0);
}

int
_zbar_window_handle_events (zbar_processor_t *proc,
                            int block)
{
    MSG msg;
    proc->input = 0;
    while(!proc->input) {
        int rc = 0;
        if(proc->display) {
            if(block)
                rc = GetMessage(&msg, proc->display, 0, 0);
            else {
                rc = PeekMessage(&msg, proc->display, 0, 0,
                                 PM_NOYIELD | PM_REMOVE);
                if(!rc)
                    break;
            }
            /*zprintf(1, "rc=%d blk=%d msg=%d\n", rc, block, msg.message);*/
            if(rc < 0) {
                proc->err.errnum = GetLastError();
                return(err_capture(proc, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                                   "failed to obtain event"));
            }
        }

        if(!rc || msg.message == WM_QUIT) {
            zprintf(1, "QUIT\n");
            _zbar_window_set_visible(proc, 0);
            return(err_capture(proc, SEV_WARNING, ZBAR_ERR_CLOSED, __func__,
                               "user closed display window"));
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return(proc->input);
}

int
_zbar_window_draw_marker(zbar_window_t *w,
                         uint32_t rgb,
                         const point_t *p)
{
    return(-1);
}

int _zbar_window_set_size (zbar_processor_t *proc,
                           unsigned width,
                           unsigned height)
{
    if(!SetWindowPos(proc->display, NULL, 0, 0, width, height,
                     SWP_ASYNCWINDOWPOS | SWP_NOACTIVATE | SWP_NOMOVE |
                     SWP_NOZORDER | SWP_NOREPOSITION))
        return(-1/*FIXME*/);

    return(0);
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
    zprintf(1, "\n");
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
    HDC hdc = GetDC(w->hwnd);
    if(!hdc || !SaveDC(hdc))
        return(-1/*FIXME*/);

    HRGN rgn = CreateRectRgn(0, 0, 0, 0);
    GetUpdateRgn(w->hwnd, rgn, 0);
    SelectClipRgn(hdc, rgn);

    SetRectRgn(rgn, 0, 0, w->width, w->height);
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
