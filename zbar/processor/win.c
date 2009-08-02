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

#include "processor.h"
#include "win.h"
#include <assert.h>

#define WIN_STYLE (WS_CAPTION | \
                   WS_SYSMENU | \
                   WS_THICKFRAME | \
                   WS_MINIMIZEBOX | \
                   WS_MAXIMIZEBOX)
#define EXT_STYLE (WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW)

static inline int proc_lock (processor_state_t *state,
                             DWORD self)
{
    if(!state->lock_level) {
        state->lock_owner = self;
        state->lock_level = 1;
        return(0);
    }

    if(state->lock_owner == self) {
        state->lock_level++;
        return(0);
    }

    wait_entry_t *entry = proc_entry_alloc(state, self);
    proc_entry_queue(&state->lock_queue, entry);

    LeaveCriticalSection(&state->mutex);
    int rc = WaitForSingleObject(entry->notify, INFINITE);
    EnterCriticalSection(&state->mutex);

    assert(state->lock_level == 1);
    assert(state->lock_owner == self);

    proc_entry_free(state, entry);
    return(rc);
}

static inline int proc_unlock (processor_state_t *state,
                               DWORD self,
                               int all)
{
    assert(state->lock_level > 0);
    assert(state->lock_owner == self);

    if(all)
        state->lock_level = 0;
    else
        state->lock_level--;

    if(!state->lock_level) {
        wait_entry_t *entry = proc_entry_dequeue(&state->lock_queue);
        if(entry) {
            state->lock_level = 1;
            state->lock_owner = entry->requester;
            SetEvent(entry->notify);
        }
    }
    return(0);
}

static inline void proc_notify (zbar_processor_t *proc)
{
    processor_state_t *state = proc->state;
    wait_entry_t **q = &state->wait_queue.head;
    while(*q) {
        wait_entry_t *entry = *q;
        if(entry->requester & proc->events) {
            *q = entry->next;
            SetEvent(entry->notify);
        }
        q = &entry->next;
        entry->next = NULL;
    }
}

inline int _zbar_processor_lock (const zbar_processor_t *proc)
{
    processor_state_t *state = proc->state;

    EnterCriticalSection(&state->mutex);
    int rc = proc_lock(state, GetCurrentThreadId());
    LeaveCriticalSection(&state->mutex);

    if(rc)
        /* FIXME save system code */
        rc = err_capture(proc, SEV_ERROR, ZBAR_ERR_LOCKING, __func__,
                         "unable to lock processor");
    return(rc);
}

inline int _zbar_processor_unlock (const zbar_processor_t *proc)
{
    processor_state_t *state = proc->state;

    EnterCriticalSection(&state->mutex);
    proc_unlock(state, GetCurrentThreadId(), 0);
    LeaveCriticalSection(&state->mutex);

    return(0);
}

inline int _zbar_processor_notify (zbar_processor_t *proc,
                                   unsigned event)
{
    processor_state_t *state = proc->state;

    EnterCriticalSection(&state->mutex);
    proc->events |= event;

    SetEvent(state->video_thread.activity);
    proc_notify(proc);

    LeaveCriticalSection(&state->mutex);
    return(0);
}

inline int _zbar_processor_wait (zbar_processor_t *proc,
                                 unsigned event,
                                 int timeout)
{
    processor_state_t *state = proc->state;
    DWORD self = GetCurrentThreadId();

    proc->events &= ~event;

    EnterCriticalSection(&state->mutex);

    int save_level = state->lock_level;
    wait_entry_t *entry = proc_entry_alloc(state, event);
    proc_entry_queue(&state->wait_queue, entry);
    proc_unlock(state, self, 1);

    LeaveCriticalSection(&state->mutex);
    int rc = WaitForSingleObject(entry->notify, timeout);
    EnterCriticalSection(&state->mutex);

    proc_entry_free(state, entry);
    proc_lock(state, self);
    state->lock_level = save_level;
    if(!rc)
        assert(proc->events & event);

    LeaveCriticalSection(&state->mutex);

    if(!rc)
        return(1); /* got event */
    if(rc == WAIT_TIMEOUT)
        return(0); /* timed out */
    return(-1); /* error (FIXME save info) */
}

static inline int proc_thread_start (zbar_processor_t *proc,
                                     thread_state_t *tstate,
                                     LPTHREAD_START_ROUTINE thread)
{
    assert(!tstate->running);
    tstate->running = 1;
    HANDLE hthr = CreateThread(NULL, 0, thread, proc, 0, NULL);
    if(!hthr) {
        tstate->running = 0;
        return(-1/*FIXME*/);
    }
    CloseHandle(hthr);

    HANDLE status[2] = { tstate->started, tstate->stopped };
    int rc = WaitForMultipleObjects(2, status, 0, INFINITE);
    if(rc) {
        tstate->running = 0;
        return(-1/*FIXME*/);
    }

    return(0);
}

static inline int proc_thread_stop (processor_state_t *state,
                                    thread_state_t *tstate)
{
    if(tstate->running) {
        tstate->running = 0;
        SetEvent(tstate->activity);
        LeaveCriticalSection(&state->mutex);
        WaitForSingleObject(tstate->stopped, INFINITE);
        EnterCriticalSection(&state->mutex);
    }
    return(0);
}

int _zbar_processor_threads_init (zbar_processor_t *proc,
                                  int threaded)
{
    if(!threaded)
        err_capture(proc, SEV_WARNING, ZBAR_ERR_UNSUPPORTED,
                    __func__, "windows processor does not support unthreaded");

    processor_state_t *state = proc->state =
        calloc(1, sizeof(processor_state_t));

    InitializeCriticalSection(&state->mutex);

    state->video_thread.activity = CreateEvent(NULL, 0, 0, NULL);
    state->video_thread.started = CreateEvent(NULL, 0, 0, NULL);
    state->video_thread.stopped = CreateEvent(NULL, 0, 0, NULL);

    state->input_thread.activity = CreateEvent(NULL, 0, 0, NULL);
    state->input_thread.started = CreateEvent(NULL, 0, 0, NULL);
    state->input_thread.stopped = CreateEvent(NULL, 0, 0, NULL);
    return(0);
}

int _zbar_processor_threads_cleanup (zbar_processor_t *proc)
{
    processor_state_t *state = proc->state;

    CloseHandle(state->video_thread.activity);
    CloseHandle(state->video_thread.started);
    CloseHandle(state->video_thread.stopped);

    CloseHandle(state->input_thread.activity);
    CloseHandle(state->input_thread.started);
    CloseHandle(state->input_thread.stopped);

    DeleteCriticalSection(&state->mutex);

    if(state->video_dev) {
        free(state->video_dev);
        state->video_dev = NULL;
    }

    free(state);
    proc->state = NULL;
    return(0);
}


static DWORD WINAPI proc_video_thread (void *arg)
{
    zbar_processor_t *proc = arg;
    processor_state_t *state = proc->state;
    thread_state_t *tstate = &state->video_thread;
    DWORD self = GetCurrentThreadId();
    int rc = 0;

    /* windows video must be opened from driving thread */
    if(zbar_video_open(proc->video, state->video_dev)) {
        err_copy(proc, proc->video);
        SetEvent(tstate->stopped);
        return(0);
    }

    EnterCriticalSection(&state->mutex);
    zprintf(4, "spawned video thread\n");
    SetEvent(tstate->started);

    wait_entry_t *entry = proc_entry_alloc(state, self);
    HANDLE activity[2] = { tstate->activity, entry->notify };

    while(tstate->running) {
        while(!proc->active && tstate->running) {
            LeaveCriticalSection(&state->mutex);
            rc = WaitForSingleObject(tstate->activity, INFINITE);
            EnterCriticalSection(&state->mutex);
        }
        if(!tstate->running)
            break;

        /* unlocked blocking image input */
        LeaveCriticalSection(&state->mutex);
        zbar_image_t *img = zbar_video_next_image(proc->video);
        EnterCriticalSection(&state->mutex);

        if(!img)
            break;

        if(!state->lock_level) {
            state->lock_owner = self;
            state->lock_level = 1;
        }
        else {
            proc_entry_queue(&state->lock_queue, entry);
            rc = 0;
            while(!rc && tstate->running) {
                LeaveCriticalSection(&state->mutex);
                rc = WaitForMultipleObjects(2, activity, 0, INFINITE);
                EnterCriticalSection(&state->mutex);
            }
            if(rc || !tstate->running)
                break;
        }

        assert(state->lock_level == 1);
        assert(state->lock_owner == self);
        LeaveCriticalSection(&state->mutex);

        if(proc->active)
            _zbar_process_image(proc, img);

        zbar_image_destroy(img);

        EnterCriticalSection(&state->mutex);
        assert(state->lock_level == 1);

        wait_entry_t *next = proc_entry_dequeue(&state->lock_queue);
        if(next) {
            state->lock_owner = next->requester;
            SetEvent(next->notify);
        }
        else
            state->lock_level = 0;
    }

    proc_entry_free(state, entry);

    LeaveCriticalSection(&state->mutex);

    SetEvent(tstate->stopped);
    return(0);
}

int _zbar_processor_threads_start (zbar_processor_t *proc,
                                   const char *dev)
{
    processor_state_t *state = proc->state;
    if(state->video_dev)
        free(state->video_dev);
    state->video_dev = strdup(dev);
    return(proc_thread_start(proc, &state->video_thread, proc_video_thread));
}

int _zbar_processor_threads_stop (zbar_processor_t *proc)
{
    processor_state_t *state = proc->state;
    EnterCriticalSection(&state->mutex);
    int rc = proc_thread_stop(proc->state, &proc->state->video_thread);
    LeaveCriticalSection(&state->mutex);
    return(rc);
}


static LRESULT CALLBACK proc_handle_event (HWND hwnd,
                                           UINT message,
                                           WPARAM wparam,
                                           LPARAM lparam)
{
    zbar_processor_t *proc =
        (zbar_processor_t*)GetWindowLongPtr(hwnd, GWL_USERDATA);
    processor_state_t *state = NULL;
    /* initialized during window creation */
    if(proc)
        state = proc->state;
    else if(message == WM_NCCREATE) {
        proc = ((LPCREATESTRUCT)lparam)->lpCreateParams;
        assert(proc);
        state = proc->state;
        assert(state);
        SetWindowLongPtr(hwnd, GWL_USERDATA, (LONG_PTR)proc);
        proc->display = state->hwnd = hwnd;

        zbar_window_attach(proc->window, proc->display, proc->xwin);
    }
    else
        return(DefWindowProc(hwnd, message, wparam, lparam));

    switch(message) {

    case WM_SIZE: {
        RECT r;
        GetClientRect(hwnd, &r);
        zprintf(3, "WM_SIZE %ldx%ld\n", r.right, r.bottom);
        assert(proc);
        zbar_window_resize(proc->window, r.right, r.bottom);
        InvalidateRect(hwnd, NULL, 0);
        return(0);
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        BeginPaint(hwnd, &ps);
        if(zbar_window_redraw(proc->window)) {
            HDC hdc = GetDC(hwnd);
            RECT r;
            GetClientRect(hwnd, &r);
            FillRect(hdc, &r, GetStockObject(BLACK_BRUSH));
            ReleaseDC(hwnd, hdc);
        }
        EndPaint(hwnd, &ps);
        return(0);
    }

    case WM_CHAR: {
        state->input = wparam;
        return(0);
    }

    case WM_LBUTTONDOWN: {
        state->input = 1;
        return(0);
    }

    case WM_MBUTTONDOWN: {
        state->input = 2;
        return(0);
    }

    case WM_RBUTTONDOWN: {
        state->input = 3;
        return(0);
    }

    case WM_CLOSE: {
        zprintf(3, "WM_CLOSE\n");
        _zbar_processor_set_visible(proc, 0);
        return(1);
    }

    case WM_DESTROY: {
        zprintf(3, "WM_DESTROY\n");
        state->hwnd = proc->display = NULL;
        zbar_window_attach(proc->window, NULL, 0);
        return(0);
    }
    }
    return(DefWindowProc(hwnd, message, wparam, lparam));
}

static inline int proc_handle_events (zbar_processor_t *proc)
{
    processor_state_t *state = proc->state;
    while(1) {
        MSG msg;
        int rc = PeekMessage(&msg, state->hwnd, 0, 0, PM_NOYIELD | PM_REMOVE);
        if(!rc)
            return(0);
        if(rc < 0)
            return(err_capture(proc, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                               "failed to obtain event"));

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if(state->input) {
            EnterCriticalSection(&state->mutex);
            proc->input = state->input;
            proc->events |= EVENT_INPUT;
            proc_notify(proc);
            LeaveCriticalSection(&state->mutex);
            state->input = 0;
        }
    }
}

static inline ATOM proc_register_class (HINSTANCE hmod)
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

static inline int proc_open (zbar_processor_t *proc,
                             unsigned width,
                             unsigned height)
{
    processor_state_t *state = proc->state;
    HMODULE hmod = NULL;
    if(!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                          GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          (void*)_zbar_processor_open, (HINSTANCE*)&hmod))
        return(err_capture(proc, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "failed to obtain module handle"));

    ATOM wca = proc_register_class(hmod);
    if(!wca)
        return(err_capture(proc, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "failed to register window class"));

    RECT r = { 0, 0, width, height };
    AdjustWindowRectEx(&r, WIN_STYLE, 0, EXT_STYLE);
    state->hwnd = CreateWindowEx(EXT_STYLE, (LPCTSTR)(long)wca,
                                 "ZBar", WIN_STYLE,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 r.right - r.left, r.bottom - r.top,
                                 NULL, NULL, hmod, proc);

    if(!state->hwnd)
        return(err_capture(proc, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "failed to open window"));
    return(0);
}

static DWORD WINAPI proc_input_thread (void *arg)
{
    zbar_processor_t *proc = arg;
    processor_state_t *state = proc->state;
    thread_state_t *tstate = &state->input_thread;
    state->input = 0;

    int rc = proc_open(proc, state->width, state->height);
    if(rc) {
        SetEvent(tstate->stopped);
        return(-1);
    }

    EnterCriticalSection(&state->mutex);
    zprintf(4, "spawned input thread\n");
    SetEvent(tstate->started);

    while(!rc && tstate->running) {
        LeaveCriticalSection(&state->mutex);

        rc = MsgWaitForMultipleObjects(1, &tstate->activity, 0,
                                       INFINITE, QS_ALLINPUT);
        if(rc == 1)
            rc = proc_handle_events(proc);

        EnterCriticalSection(&state->mutex);
    }
    LeaveCriticalSection(&state->mutex);

    if(state->hwnd) {
        DestroyWindow(state->hwnd);
        state->hwnd = proc->display = NULL;
    }

    SetEvent(tstate->stopped);
    return(0);
}


int _zbar_processor_open (zbar_processor_t *proc,
                          char *title,
                          unsigned width,
                          unsigned height)
{
    processor_state_t *state = proc->state;
    state->width = width;
    state->height = height;
    return(proc_thread_start(proc, &state->input_thread,
                             proc_input_thread));
}

int _zbar_processor_close (zbar_processor_t *proc)
{
    processor_state_t *state = proc->state;
    EnterCriticalSection(&state->mutex);
    int rc = proc_thread_stop(proc->state, &proc->state->input_thread);
    LeaveCriticalSection(&state->mutex);
    return(rc);
}

int _zbar_processor_set_visible (zbar_processor_t *proc,
                                 int visible)
{
    processor_state_t *state = proc->state;
    ShowWindow(state->hwnd, (visible) ? SW_SHOWNORMAL : SW_HIDE);
    if(visible)
        InvalidateRect(state->hwnd, NULL, 0);
    else {
        EnterCriticalSection(&state->mutex);
        if(!(proc->events & EVENT_INPUT)) {
            proc->input = err_capture(proc, SEV_WARNING, ZBAR_ERR_CLOSED,
                                      __func__, "user closed display window");
            proc->events |= EVENT_INPUT;
        }
        proc_notify(proc);
        LeaveCriticalSection(&state->mutex);
    }
    proc->visible = (visible != 0);
    /* no error conditions */
    return(0);
}

int _zbar_processor_set_size (zbar_processor_t *proc,
                              unsigned width,
                              unsigned height)
{
    RECT r = { 0, 0, width, height };
    processor_state_t *state = proc->state;
    int rc = AdjustWindowRectEx(&r, GetWindowLong(state->hwnd, GWL_STYLE),
                                0, GetWindowLong(state->hwnd, GWL_EXSTYLE));
    zprintf(5, "%dx%d %ld,%ld-%ld,%ld (%d)\n", width, height,
            r.left, r.top, r.right, r.bottom, rc);
    if(!SetWindowPos(state->hwnd, NULL,
                     0, 0, r.right - r.left, r.bottom - r.top,
                     SWP_NOACTIVATE | SWP_NOMOVE |
                     SWP_NOZORDER | SWP_NOREPOSITION))
        return(-1/*FIXME*/);

    return(0);
}

int _zbar_processor_invalidate (zbar_processor_t *proc)
{
    processor_state_t *state = proc->state;
    if(!InvalidateRect(state->hwnd, NULL, 0))
        return(-1/*FIXME*/);

    return(0);
}

int _zbar_processor_enable (zbar_processor_t *proc,
                            int active)
{
    return(0);
}
