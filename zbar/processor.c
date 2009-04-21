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
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>   /* gettimeofday */
#include <assert.h>
#include <errno.h>

#ifdef HAVE_LIBPTHREAD

static inline void proc_mutex_unlock (void *arg)
{
    pthread_mutex_unlock((pthread_mutex_t*)arg);
}

/* acquire window access semaphore */
static inline int proc_lock (zbar_processor_t *proc)
{
    if(!proc->threaded)
        return(0);
    int rc = pthread_mutex_lock(&proc->mutex);
    pthread_cleanup_push(proc_mutex_unlock, &proc->mutex);
    while(!rc && !proc->sem)
        rc = pthread_cond_wait(&proc->cond, &proc->mutex);
    if(!rc) {
        proc->sem--;
        proc->sem_owner = pthread_self();
    }
    pthread_cleanup_pop(1);
    if(rc)
        err_capture(proc, SEV_ERROR, ZBAR_ERR_LOCKING, __func__,
                    "unable to lock processor");
    return(rc);
}

/* release window access semaphore */
static inline void proc_unlock (zbar_processor_t *proc)
{
    if(!proc->threaded)
        return;
    assert(!proc->sem);
    assert(pthread_equal(proc->sem_owner, pthread_self()));
    pthread_mutex_lock(&proc->mutex);
    proc->sem++;
    pthread_cond_signal(&proc->cond);
    pthread_mutex_unlock(&proc->mutex);
}

static inline void proc_destroy_thread (pthread_t thread,
                                        unsigned *started)
{
    if(*started) {
        pthread_cancel(thread);
        pthread_join(thread, NULL);
        *started = 0;
    }
}

#else
# define proc_lock(...) (0)
# define proc_unlock(...)
# define proc_destroy_thread(...)
#endif

/* lock is already held */
static inline int process_image (zbar_processor_t *proc,
                                 zbar_image_t *img)
{
    if(img) {
        uint32_t format = zbar_image_get_format(img);
        zprintf(16, "processing: %.4s(%08" PRIx32 ") %dx%d @%p\n",
                (char*)&format, format,
                zbar_image_get_width(img), zbar_image_get_height(img),
                zbar_image_get_data(img));

        /* FIXME locking all other interfaces while processing is conservative
         * but easier for now and we don't expect this to take long...
         */
        int nsyms = zbar_scan_image(proc->scanner, img);
        if(nsyms < 0)
            return(err_capture(proc, SEV_ERROR, ZBAR_ERR_UNSUPPORTED,
                               __func__, "unknown image format"));

        if(_zbar_verbosity >= 8) {
            const zbar_symbol_t *sym = zbar_image_first_symbol(img);
            while(sym) {
                zbar_symbol_type_t type = zbar_symbol_get_type(sym);
                int count = zbar_symbol_get_count(sym);
                zprintf(8, "%s%s: %s (%s)\n",
                        zbar_get_symbol_name(type),
                        zbar_get_addon_name(type),
                        zbar_symbol_get_data(sym),
                        (count < 0) ? "uncertain" :
                        (count > 0) ? "duplicate" : "new");
                sym = zbar_symbol_next(sym);
            }
        }

        if(nsyms) {
            emit(proc, EVENT_OUTPUT);
            if(proc->handler)
                /* FIXME only call after filtering (unlocked...?) */
                proc->handler(img, proc->userdata);
        }

        if(proc->force_output) {
            img = zbar_image_convert(img, proc->force_output);
            if(!img)
                return(err_capture(proc, SEV_ERROR, ZBAR_ERR_UNSUPPORTED,
                                   __func__, "unknown image format"));
        }
    }

    /* display to window if enabled */
    if(proc->window &&
       (zbar_window_draw(proc->window, img) ||
        _zbar_window_invalidate(proc->window)))
        return(err_copy(proc, proc->window));
#if 0
    /* FIXME still don't understand why we need this */
    if(proc->window && _zbar_window_handle_events(proc, 0))
        return(-1);
#endif
    if(proc->force_output && img)
        zbar_image_destroy(img);
    return(0);
}

/* used by poll interface.  lock is already held */
static int proc_video_handler (zbar_processor_t *proc,
                               int i)
{
    if(!proc->active)
        return(0);
    /* not expected to block */
    zbar_image_t *img = zbar_video_next_image(proc->video);
    if(!img)
        return(-1);
    int rc = process_image(proc, img);
    zbar_image_destroy(img);
    return(rc);
}

static inline int proc_poll_inputs (zbar_processor_t *proc,
                                    poll_desc_t *p,
                                    int timeout)
{
    assert(p->num);
    int rc = poll(p->fds, p->num, timeout);
    if(rc <= 0)
        /* FIXME detect and handle fatal errors (somehow) */
        return(rc);
    (void)proc_lock(proc);
    if(p->fds[0].revents && p->fds[0].fd == proc->kick_fds[0]) {
        unsigned junk[2];
        read(proc->kick_fds[0], junk, 2 * sizeof(unsigned));
        proc_unlock(proc);
        return(1);
    }
    int i;
    for(i = 0; i < p->num; i++)
        if(p->fds[i].revents) {
            if(p->handlers[i])
                p->handlers[i](proc, i);
            p->fds[i].revents = 0; /* debug */
            rc--;
        }
    proc_unlock(proc);
    assert(!rc);
    return(0);
}

static inline int proc_calc_abstime (struct timespec *abstime,
                                     int timeout)
{
    if(timeout >= 0) {
#if _POSIX_TIMERS > 0
        clock_gettime(CLOCK_REALTIME, abstime);
#else
        struct timeval ustime;
        gettimeofday(&ustime, NULL);
        abstime->tv_nsec = ustime.tv_usec * 1000;
        abstime->tv_sec = ustime.tv_sec;
#endif
        abstime->tv_nsec += (timeout % 1000) * 1000000;
        abstime->tv_sec += (timeout / 1000) + (abstime->tv_nsec / 1000000000);
        abstime->tv_nsec %= 1000000000;
    }
    return(0);
}

#ifdef HAVE_LIBPTHREAD
static inline int proc_event_wait_threaded (zbar_processor_t *proc,
                                            unsigned event,
                                            int timeout,
                                            struct timespec *abstime)
{
    assert(!proc->sem);
    assert(pthread_equal(proc->sem_owner, pthread_self()));
    int rc = pthread_mutex_lock(&proc->mutex);
    proc->events &= ~event;
    proc->sem++;
    pthread_cond_signal(&proc->cond);
    pthread_cleanup_push(proc_mutex_unlock, &proc->mutex);
    while(!rc && !(proc->events & event))
        if(timeout < 0)
            rc = pthread_cond_wait(&proc->event, &proc->mutex);
        else
            rc = pthread_cond_timedwait(&proc->event, &proc->mutex, abstime);
    if(rc == ETIMEDOUT)
        rc = 0;
    while(!rc && !proc->sem)
        rc = pthread_cond_wait(&proc->cond, &proc->mutex);
    proc->sem--;
    proc->sem_owner = pthread_self();
    pthread_cleanup_pop(1);
    if(rc)
        return(-1); /* error */
    if(proc->events & event)
        return(1); /* got event */
    return(0); /* timed out */
}
#endif

static inline int proc_event_wait_unthreaded (zbar_processor_t *proc,
                                              unsigned event,
                                              int timeout,
                                              struct timespec *abstime)
{
    int blocking = proc->active && zbar_video_get_fd(proc->video) < 0;
    proc->events &= ~event;
    /* unthreaded, poll here for window input */
    while(!(proc->events & event)) {
        if(blocking) {
            zbar_image_t *img = zbar_video_next_image(proc->video);
            if(!img)
                return(-1);
            process_image(proc, img);
            zbar_image_destroy(img);
        }
        int reltime = timeout;
        if(reltime >= 0) {
            struct timespec now;
#if _POSIX_TIMERS > 0
            clock_gettime(CLOCK_REALTIME, &now);
#else
            struct timeval ustime;
            gettimeofday(&ustime, NULL);
            now.tv_nsec = ustime.tv_usec * 1000;
            now.tv_sec = ustime.tv_sec;
#endif
            reltime = ((abstime->tv_sec - now.tv_sec) * 1000 +
                       (abstime->tv_nsec - now.tv_nsec) / 1000000);
            if(reltime <= 0)
                return(0);
        }
        if(blocking && (reltime < 0 || reltime > 10))
            reltime = 10;
        if(proc->polling.num)
            proc_poll_inputs(proc, (poll_desc_t*)&proc->polling, reltime);
        else if(!blocking) {
            proc_unlock(proc);
            struct timespec sleepns, remns;
            sleepns.tv_sec = timeout / 1000;
            sleepns.tv_nsec = (timeout % 1000) * 1000000;
            while(nanosleep(&sleepns, &remns) && errno == EINTR)
                sleepns = remns;
            (void)proc_lock(proc);
            return(0);
        }
    }
    return(1);
}

static inline int proc_event_wait (zbar_processor_t *proc,
                                   unsigned event,
                                   int timeout)
{
    struct timespec abstime;
    proc_calc_abstime(&abstime, timeout);
#ifdef HAVE_LIBPTHREAD
    if(proc->threaded)
        return(proc_event_wait_threaded(proc, event, timeout, &abstime));
    else
#endif
        return(proc_event_wait_unthreaded(proc, event, timeout, &abstime));
}

#ifdef HAVE_LIBPTHREAD
/* make a thread-local copy of polling data */
static inline void proc_cache_polling (zbar_processor_t *proc)
{
    (void)proc_lock(proc);
    int n = proc->polling.num;
    zprintf(5, "%d fds\n", n);
    proc->thr_polling.num = n;
    alloc_polls(&proc->thr_polling);
    memcpy(proc->thr_polling.fds, proc->polling.fds,
           n * sizeof(struct pollfd));
    memcpy(proc->thr_polling.handlers, proc->polling.handlers,
           n * sizeof(poll_handler_t*));
    proc_unlock(proc);
}

static inline void proc_block_sigs ()
{
    sigset_t sigs;
    sigfillset(&sigs);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);
}

/* input handler thread to poll available inputs */
static void *proc_input_thread (void *arg)
{
    zbar_processor_t *proc = arg;
    proc_block_sigs();

    /* wait for registered inputs and process using associated handler */
    while(1) {
        proc_cache_polling(proc);

        int rc = 0;
        while(!rc) {
            if(proc->polling.num)
                rc = proc_poll_inputs(proc, &proc->thr_polling, -1);
            else {
                struct timespec t;
                t.tv_sec = 4;
                t.tv_nsec = 0;
                rc = nanosleep(&t, NULL);
            }
        }
    }
}

/* separate thread to handle non-pollable video (v4l1) */
static void *proc_video_thread (void *arg)
{
    zbar_processor_t *proc = arg;
    proc_block_sigs();

    (void)proc_lock(proc);
    while(1) {
        /* wait for active to be set */
        assert(!proc->sem);
        assert(pthread_equal(proc->sem_owner, pthread_self()));
        pthread_mutex_lock(&proc->mutex);
        proc->sem++;
        pthread_cond_signal(&proc->cond);
        pthread_cleanup_push(proc_mutex_unlock, &proc->mutex);
        while(!proc->active)
            pthread_cond_wait(&proc->event, &proc->mutex);
        pthread_cleanup_pop(1);

        /* unlocked blocking image input */
        zbar_image_t *img = zbar_video_next_image(proc->video);
        if(!img)
            return(NULL);
        (void)proc_lock(proc);
        if(proc->active)
            process_image(proc, img);
        zbar_image_destroy(img);
    }
}
#endif

zbar_processor_t *zbar_processor_create (int threaded)
{
    zbar_processor_t *proc = calloc(1, sizeof(zbar_processor_t));
    if(!proc)
        return(NULL);
    err_init(&proc->err, ZBAR_MOD_PROCESSOR);
    proc->kick_fds[0] = proc->kick_fds[1] = -1;

    proc->scanner = zbar_image_scanner_create();
    if(!proc->scanner) {
        free(proc);
        return(NULL);
    }

    if(threaded) {
#ifdef HAVE_LIBPTHREAD
        proc->threaded = 1;
        proc->sem = 1;
        /* FIXME check errors */
        pthread_mutex_init(&proc->mutex, NULL);
        pthread_cond_init(&proc->cond, NULL);
        pthread_cond_init(&proc->event, NULL);
        pipe(proc->kick_fds);
        add_poll(proc, proc->kick_fds[0], NULL);
#else
    /* FIXME record warning */
#endif
    }

    return(proc);
}

void zbar_processor_destroy (zbar_processor_t *proc)
{
    (void)proc_lock(proc);
    proc_destroy_thread(proc->video_thread, &proc->video_started);
    proc_destroy_thread(proc->input_thread, &proc->input_started);
    if(proc->window) {
        zbar_window_destroy(proc->window);
        proc->window = NULL;
        _zbar_window_close(proc);
    }
    if(proc->video) {
        zbar_video_destroy(proc->video);
        proc->video = NULL;
    }
    if(proc->scanner) {
        zbar_image_scanner_destroy(proc->scanner);
        proc->scanner = NULL;
    }
    proc_unlock(proc);
    err_cleanup(&proc->err);
#ifdef HAVE_LIBPTHREAD
    pthread_cond_destroy(&proc->event);
    pthread_cond_destroy(&proc->cond);
    pthread_mutex_destroy(&proc->mutex);
#endif
    if(proc->polling.fds) {
        free(proc->polling.fds);
        proc->polling.fds = NULL;
    }
    if(proc->polling.handlers) {
        free(proc->polling.handlers);
        proc->polling.handlers = NULL;
    }
    if(proc->thr_polling.fds) {
        free(proc->thr_polling.fds);
        proc->thr_polling.fds = NULL;
    }
    if(proc->thr_polling.handlers) {
        free(proc->thr_polling.handlers);
        proc->thr_polling.handlers = NULL;
    }
    free(proc);
}

int zbar_processor_init (zbar_processor_t *proc,
                         const char *dev,
                         int enable_display)
{
    if(proc_lock(proc) < 0)
        return(-1);
    proc_destroy_thread(proc->video_thread, &proc->video_started);
    proc_destroy_thread(proc->input_thread, &proc->input_started);
    proc->video_started = proc->input_started = 0;
    int rc = 0;
    if(proc->video) {
        if(dev)
            zbar_video_open(proc->video, NULL);
        else {
            zbar_video_destroy(proc->video);
            proc->video = NULL;
        }
    }
    if(proc->window) {
        zbar_window_destroy(proc->window);
        proc->window = NULL;
        _zbar_window_close(proc);
    }

    if(!dev && !enable_display)
        /* nothing to do */
        goto done;

    if(enable_display) {
        proc->window = zbar_window_create();
        if(!proc->window) {
            rc = err_capture(proc, SEV_FATAL, ZBAR_ERR_NOMEM,
                             __func__, "allocating window resources");
            goto done;
        }
    }

    if(dev) {
        proc->video = zbar_video_create();
        if(!proc->video) {
            rc = err_capture(proc, SEV_FATAL, ZBAR_ERR_NOMEM,
                             __func__, "allocating video resources");
            goto done;
        }
        if(proc->req_width || proc->req_height)
            zbar_video_request_size(proc->video,
                                     proc->req_width, proc->req_height);
        if(proc->req_v4l)
            zbar_video_request_interface(proc->video, proc->req_v4l);
        if((proc->req_iomode &&
            zbar_video_request_iomode(proc->video, proc->req_iomode)) ||
           zbar_video_open(proc->video, dev)) {
            rc = err_copy(proc, proc->video);
            goto done;
        }
        if(zbar_video_get_fd(proc->video) < 0 && proc->threaded) {
#ifdef HAVE_LIBPTHREAD
            /* spawn blocking video thread */
            if((rc = pthread_create(&proc->video_thread, NULL,
                                    proc_video_thread, proc))) {
                rc = err_capture_num(proc, SEV_ERROR, ZBAR_ERR_SYSTEM,
                                     __func__, "spawning video thread", rc);
                goto done;
            }
            proc->video_started = 1;
            zprintf(4, "spawned video thread\n");
#endif
        }
    }

#ifdef HAVE_LIBPTHREAD
    if(proc->threaded && (proc->window ||
                          (proc->video && !proc->video_started))) {
        /* spawn input monitor thread */
        if((rc = pthread_create(&proc->input_thread, NULL,
                                proc_input_thread, proc))) {
            rc = err_capture_num(proc, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                                 "spawning input thread", rc);
            goto done;
        }
        proc->input_started = 1;
        zprintf(4, "spawned input thread\n");
    }
#endif

    if(proc->window) {
        /* arbitrary default */
        int width = 640, height = 480;
        if(proc->video) {
            width = zbar_video_get_width(proc->video);
            height = zbar_video_get_height(proc->video);
        }

        if(_zbar_window_open(proc, "zbar barcode reader", width, height)) {
            rc = -1;
            goto done;
        }
        else if(zbar_window_attach(proc->window, proc->display, proc->xwin)) {
            rc = err_copy(proc, proc->window);
            goto done;
        }
    }

    if(proc->video && proc->force_input) {
        if(zbar_video_init(proc->video, proc->force_input))
            rc = err_copy(proc, proc->video);
    }
    else if(proc->video)
        while(zbar_negotiate_format(proc->video, proc->window)) {
            if(proc->video && proc->window) {
                fprintf(stderr,
                        "WARNING: no compatible input to output format\n"
                        "...trying again with output disabled\n");
                zbar_window_destroy(proc->window);
                proc->window = NULL;
            }
            else {
                zprintf(1, "ERROR: no compatible %s format\n",
                        (proc->video) ? "video input" : "window output");
                rc = err_capture(proc, SEV_ERROR, ZBAR_ERR_UNSUPPORTED,
                                 __func__, "no compatible image format");
                goto done;
            }
        }

 done:
    proc_unlock(proc);
    return(rc);
}

zbar_image_data_handler_t*
zbar_processor_set_data_handler (zbar_processor_t *proc,
                                 zbar_image_data_handler_t *handler,
                                 const void *userdata)
{
    if(proc_lock(proc) < 0)
        return(NULL);
    zbar_image_data_handler_t *result = proc->handler;
    proc->handler = handler;
    proc->userdata = userdata;
    proc_unlock(proc);
    return(result);
}

void zbar_processor_set_userdata (zbar_processor_t *proc,
                                  void *userdata)
{
    if(proc_lock(proc) < 0)
        return;
    proc->userdata = userdata;
    proc_unlock(proc);
}

void *zbar_processor_get_userdata (const zbar_processor_t *proc)
{
    return((void*)proc->userdata);
}

int zbar_processor_set_config (zbar_processor_t *proc,
                               zbar_symbol_type_t sym,
                               zbar_config_t cfg,
                               int val)
{
    return(zbar_image_scanner_set_config(proc->scanner, sym, cfg, val));
}

int zbar_processor_request_size (zbar_processor_t *proc,
                                 unsigned width,
                                 unsigned height)
{
    proc->req_width = width;
    proc->req_height = height;
    return(0);
}

int zbar_processor_request_interface (zbar_processor_t *proc,
                                      int ver)
{
    proc->req_v4l = ver;
    return(0);
}

int zbar_processor_request_iomode (zbar_processor_t *proc,
                                   int iomode)
{
    proc->req_iomode = iomode;
    return(0);
}

int zbar_processor_force_format (zbar_processor_t *proc,
                                 unsigned long input,
                                 unsigned long output)
{
    proc->force_input = input;
    proc->force_output = output;
    return(0);
}

int zbar_processor_is_visible (zbar_processor_t *proc)
{
    if(proc_lock(proc) < 0)
        return(-1);
    int visible = proc->window && proc->visible;
    proc_unlock(proc);
    return(visible);
}

int zbar_processor_set_visible (zbar_processor_t *proc,
                                int visible)
{
    if(proc_lock(proc) < 0)
        return(-1);
    int rc = 0;
    if(proc->window) {
        if(proc->video)
            rc = _zbar_window_set_size(proc,
                                       zbar_video_get_width(proc->video),
                                       zbar_video_get_height(proc->video));
        if(!rc)
            rc = _zbar_window_set_visible(proc, visible);
    }
    else if(visible)
        rc = err_capture(proc, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                         "processor display window not initialized");
    proc_unlock(proc);
    return(rc);
}

int zbar_processor_user_wait (zbar_processor_t *proc,
                              int timeout)
{
    if(proc_lock(proc) < 0)
        return(-1);
    int rc = -1;
    if(proc->visible || proc->active || timeout > 0)
        rc = proc_event_wait(proc, EVENT_INPUT, timeout);
    if(rc > 0)
        rc = proc->input;
    proc_unlock(proc);
    if(!proc->visible)
        return(err_capture(proc, SEV_WARNING, ZBAR_ERR_CLOSED, __func__,
                           "display window not available for input"));
    return(rc);
}

int zbar_processor_set_active (zbar_processor_t *proc,
                               int active)
{
    if(proc_lock(proc) < 0)
        return(-1);
    if(!proc->video) {
        proc_unlock(proc);
        return(-1);
    }
    zbar_image_scanner_enable_cache(proc->scanner, active);

    int rc = zbar_video_enable(proc->video, active);
    if(!rc) {
        int vid_fd = zbar_video_get_fd(proc->video);
        if(vid_fd >= 0) {
            if(active)
                add_poll(proc, vid_fd, proc_video_handler);
            else
                remove_poll(proc, vid_fd);
        }
        /* FIXME failure recovery? */
        proc->active = active;
        proc->events &= ~EVENT_INPUT;
    }
    else
        err_copy(proc, proc->video);

    if(!active && proc->window &&
       (zbar_window_draw(proc->window, NULL) ||
        _zbar_window_invalidate(proc->window)) && !rc)
        rc = err_copy(proc, proc->window);

#ifdef HAVE_LIBPTHREAD
    if(proc->threaded) {
        assert(!proc->sem);
        assert(pthread_equal(proc->sem_owner, pthread_self()));
        pthread_mutex_lock(&proc->mutex);
        proc->sem++;
        pthread_cond_broadcast(&proc->event);
        pthread_cond_signal(&proc->cond);
        pthread_mutex_unlock(&proc->mutex);
    }
#endif
    return(rc);
}

int zbar_process_one (zbar_processor_t *proc,
                      int timeout)
{
    if(proc_lock(proc) < 0)
        return(-1);
    int rc = 0;
    if(proc->video) {
        zbar_image_scanner_enable_cache(proc->scanner, 1);
        rc = zbar_video_enable(proc->video, 1);
        /* FIXME failure recovery? */
        int vid_fd = zbar_video_get_fd(proc->video);
        if(vid_fd >= 0)
            add_poll(proc, vid_fd, proc_video_handler);
        proc->active = 1;
#ifdef HAVE_LIBPTHREAD
        pthread_cond_broadcast(&proc->event);
#endif
        proc_event_wait(proc, EVENT_OUTPUT, timeout);
        rc = zbar_video_enable(proc->video, 0);
        if(vid_fd >= 0)
            remove_poll(proc, vid_fd);
        proc->active = 0;
        proc->events &= ~EVENT_INPUT;
        zbar_image_scanner_enable_cache(proc->scanner, 0);
    }
    else
        rc = -1;
    proc_unlock(proc);
    return(rc);
}

int zbar_process_image (zbar_processor_t *proc,
                        zbar_image_t *img)
{
    if(proc_lock(proc) < 0)
        return(-1);
    int rc = 0;
    if(img && proc->window)
        rc = _zbar_window_set_size(proc,
                                   zbar_image_get_width(img),
                                   zbar_image_get_height(img));
    if(!rc) {
        zbar_image_scanner_enable_cache(proc->scanner, 0);
        rc = process_image(proc, img);
        if(proc->active)
            zbar_image_scanner_enable_cache(proc->scanner, 1);
    }
    proc_unlock(proc);
    return(rc);
}
