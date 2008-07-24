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
static inline int proc_lock (zebra_processor_t *proc)
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
        err_capture(proc, SEV_ERROR, ZEBRA_ERR_LOCKING, __func__,
                    "unable to lock processor");
    return(rc);
}

/* release window access semaphore */
static inline void proc_unlock (zebra_processor_t *proc)
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
static inline int process_image (zebra_processor_t *proc,
                                 zebra_image_t *img)
{
    if(img) {
        uint32_t format = zebra_image_get_format(img);
        zprintf(16, "processing: %.4s(%08" PRIx32 ") %dx%d @%p\n",
                (char*)&format, format,
                zebra_image_get_width(img), zebra_image_get_height(img),
                zebra_image_get_data(img));

        /* FIXME locking all other interfaces while processing is conservative
         * but easier for now and we don't expect this to take long...
         */
        int nsyms = zebra_scan_image(proc->scanner, img);
        if(nsyms < 0)
            return(err_capture(proc, SEV_ERROR, ZEBRA_ERR_UNSUPPORTED,
                               __func__, "unknown image format"));

        if(_zebra_verbosity >= 8) {
            const zebra_symbol_t *sym = zebra_image_first_symbol(img);
            while(sym) {
                zebra_symbol_type_t type = zebra_symbol_get_type(sym);
                int count = zebra_symbol_get_count(sym);
                zprintf(8, "%s%s: %s (%s)\n",
                        zebra_get_symbol_name(type),
                        zebra_get_addon_name(type),
                        zebra_symbol_get_data(sym),
                        (count < 0) ? "uncertain" :
                        (count > 0) ? "duplicate" : "new");
                sym = zebra_symbol_next(sym);
            }
        }

        if(nsyms) {
            emit(proc, EVENT_OUTPUT);
            if(proc->handler)
                /* FIXME only call after filtering (unlocked...?) */
                proc->handler(img, proc->userdata);
        }

        if(proc->force_output) {
            img = zebra_image_convert(img, proc->force_output);
            if(!img)
                return(err_capture(proc, SEV_ERROR, ZEBRA_ERR_UNSUPPORTED,
                                   __func__, "unknown image format"));
        }
    }

    /* display to window if enabled */
    if(proc->window &&
       (zebra_window_draw(proc->window, img) ||
        _zebra_window_invalidate(proc->window)))
        return(err_copy(proc, proc->window));
#if 0
    /* FIXME still don't understand why we need this */
    if(proc->window && _zebra_window_handle_events(proc, 0))
        return(-1);
#endif
    if(proc->force_output && img)
        zebra_image_destroy(img);
    return(0);
}

/* used by poll interface.  lock is already held */
static int proc_video_handler (zebra_processor_t *proc,
                               int i)
{
    if(!proc->active)
        return(0);
    /* not expected to block */
    zebra_image_t *img = zebra_video_next_image(proc->video);
    if(!img)
        return(-1);
    int rc = process_image(proc, img);
    zebra_image_destroy(img);
    return(rc);
}

static inline int proc_poll_inputs (zebra_processor_t *proc,
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
static inline int proc_event_wait_threaded (zebra_processor_t *proc,
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

static inline int proc_event_wait_unthreaded (zebra_processor_t *proc,
                                              unsigned event,
                                              int timeout,
                                              struct timespec *abstime)
{
    int blocking = proc->active && zebra_video_get_fd(proc->video) < 0;
    proc->events &= ~event;
    /* unthreaded, poll here for window input */
    while(!(proc->events & event)) {
        if(blocking) {
            zebra_image_t *img = zebra_video_next_image(proc->video);
            if(!img)
                return(-1);
            process_image(proc, img);
            zebra_image_destroy(img);
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

static inline int proc_event_wait (zebra_processor_t *proc,
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
static inline void proc_cache_polling (zebra_processor_t *proc)
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
    zebra_processor_t *proc = arg;
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
    zebra_processor_t *proc = arg;
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
        zebra_image_t *img = zebra_video_next_image(proc->video);
        if(!img)
            return(NULL);
        (void)proc_lock(proc);
        if(proc->active)
            process_image(proc, img);
        zebra_image_destroy(img);
    }
}
#endif

zebra_processor_t *zebra_processor_create (int threaded)
{
    zebra_processor_t *proc = calloc(1, sizeof(zebra_processor_t));
    if(!proc)
        return(NULL);
    err_init(&proc->err, ZEBRA_MOD_PROCESSOR);
    proc->kick_fds[0] = proc->kick_fds[1] = -1;

    proc->scanner = zebra_image_scanner_create();
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

void zebra_processor_destroy (zebra_processor_t *proc)
{
    (void)proc_lock(proc);
    proc_destroy_thread(proc->video_thread, &proc->video_started);
    proc_destroy_thread(proc->input_thread, &proc->input_started);
    if(proc->window) {
        zebra_window_destroy(proc->window);
        proc->window = NULL;
        _zebra_window_close(proc);
    }
    if(proc->video) {
        zebra_video_destroy(proc->video);
        proc->video = NULL;
    }
    if(proc->scanner) {
        zebra_image_scanner_destroy(proc->scanner);
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

int zebra_processor_init (zebra_processor_t *proc,
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
            zebra_video_open(proc->video, NULL);
        else {
            zebra_video_destroy(proc->video);
            proc->video = NULL;
        }
    }
    if(proc->window) {
        zebra_window_destroy(proc->window);
        proc->window = NULL;
        _zebra_window_close(proc);
    }

    if(!dev && !enable_display)
        /* nothing to do */
        goto done;

    if(enable_display) {
        proc->window = zebra_window_create();
        if(!proc->window) {
            rc = err_copy(proc, proc->window);
            goto done;
        }
    }

    if(dev) {
        proc->video = zebra_video_create();
        if(!proc->video ||
           zebra_video_open(proc->video, dev)) {
            rc = err_copy(proc, proc->video);
            goto done;
        }
        if(zebra_video_get_fd(proc->video) < 0 && proc->threaded) {
#ifdef HAVE_LIBPTHREAD
            /* spawn blocking video thread */
            if((rc = pthread_create(&proc->video_thread, NULL,
                                    proc_video_thread, proc))) {
                rc = err_capture_num(proc, SEV_ERROR, ZEBRA_ERR_SYSTEM,
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
            rc = err_capture_num(proc, SEV_ERROR, ZEBRA_ERR_SYSTEM, __func__,
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
            width = zebra_video_get_width(proc->video);
            height = zebra_video_get_height(proc->video);
        }

        if(_zebra_window_open(proc, "zebra barcode reader", width, height)) {
            rc = -1;
            goto done;
        }
        else if(zebra_window_attach(proc->window, proc->display, proc->xwin)) {
            rc = err_copy(proc, proc->window);
            goto done;
        }
    }

    if(proc->video && proc->force_input) {
        if(zebra_video_init(proc->video, proc->force_input))
            rc = err_copy(proc, proc->video);
    }
    else if(proc->video)
        while(zebra_negotiate_format(proc->video, proc->window)) {
            if(proc->video && proc->window) {
                fprintf(stderr,
                        "WARNING: no compatible input to output format\n"
                        "...trying again with output disabled\n");
                zebra_window_destroy(proc->window);
                proc->window = NULL;
            }
            else {
                zprintf(1, "ERROR: no compatible %s format\n",
                        (proc->video) ? "video input" : "window output");
                rc = err_capture(proc, SEV_ERROR, ZEBRA_ERR_UNSUPPORTED,
                                 __func__, "no compatible image format");
                goto done;
            }
        }

 done:
    proc_unlock(proc);
    return(rc);
}

zebra_image_data_handler_t*
zebra_processor_set_data_handler (zebra_processor_t *proc,
                                  zebra_image_data_handler_t *handler,
                                  const void *userdata)
{
    if(proc_lock(proc) < 0)
        return(NULL);
    zebra_image_data_handler_t *result = proc->handler;
    proc->handler = handler;
    proc->userdata = userdata;
    proc_unlock(proc);
    return(result);
}

int zebra_processor_set_config (zebra_processor_t *proc,
                                zebra_symbol_type_t sym,
                                zebra_config_t cfg,
                                int val)
{
    return(zebra_image_scanner_set_config(proc->scanner, sym, cfg, val));
}

int zebra_processor_force_format (zebra_processor_t *proc,
                                   unsigned long input,
                                   unsigned long output)
{
    proc->force_input = input;
    proc->force_output = output;
    return(0);
}

int zebra_processor_is_visible (zebra_processor_t *proc)
{
    if(proc_lock(proc) < 0)
        return(-1);
    int visible = proc->window && proc->visible;
    proc_unlock(proc);
    return(visible);
}

int zebra_processor_set_visible (zebra_processor_t *proc,
                                 int visible)
{
    if(proc_lock(proc) < 0)
        return(-1);
    int rc = 0;
    if(proc->window) {
        if(proc->video)
            rc = _zebra_window_resize(proc,
                                      zebra_video_get_width(proc->video),
                                      zebra_video_get_height(proc->video));
        if(!rc)
            rc = _zebra_window_set_visible(proc, visible);
    }
    else if(visible)
        rc = err_capture(proc, SEV_ERROR, ZEBRA_ERR_INVALID, __func__,
                         "processor display window not initialized");
    proc_unlock(proc);
    return(rc);
}

int zebra_processor_user_wait (zebra_processor_t *proc,
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
        return(err_capture(proc, SEV_WARNING, ZEBRA_ERR_CLOSED, __func__,
                           "display window not available for input"));
    return(rc);
}

int zebra_processor_set_active (zebra_processor_t *proc,
                                int active)
{
    if(proc_lock(proc) < 0)
        return(-1);
    if(!proc->video) {
        proc_unlock(proc);
        return(-1);
    }
    zebra_image_scanner_enable_cache(proc->scanner, active);
    int rc = zebra_video_enable(proc->video, active);
    int vid_fd = zebra_video_get_fd(proc->video);
    if(vid_fd >= 0) {
        if(active)
            add_poll(proc, vid_fd, proc_video_handler);
        else
            remove_poll(proc, vid_fd);
    }
    /* FIXME failure recovery? */
    proc->active = active;
    proc->events &= ~EVENT_INPUT;
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

int zebra_process_one (zebra_processor_t *proc,
                       int timeout)
{
    if(proc_lock(proc) < 0)
        return(-1);
    int rc = 0;
    if(proc->video) {
        zebra_image_scanner_enable_cache(proc->scanner, 1);
        rc = zebra_video_enable(proc->video, 1);
        /* FIXME failure recovery? */
        int vid_fd = zebra_video_get_fd(proc->video);
        if(vid_fd >= 0)
            add_poll(proc, vid_fd, proc_video_handler);
        proc->active = 1;
#ifdef HAVE_LIBPTHREAD
        pthread_cond_broadcast(&proc->event);
#endif
        proc_event_wait(proc, EVENT_OUTPUT, timeout);
        rc = zebra_video_enable(proc->video, 0);
        if(vid_fd >= 0)
            remove_poll(proc, vid_fd);
        proc->active = 0;
        proc->events &= ~EVENT_INPUT;
        zebra_image_scanner_enable_cache(proc->scanner, 0);
    }
    else
        rc = -1;
    proc_unlock(proc);
    return(rc);
}

int zebra_process_image (zebra_processor_t *proc,
                         zebra_image_t *img)
{
    if(proc_lock(proc) < 0)
        return(-1);
    int rc = 0;
    if(img && proc->window)
        rc = _zebra_window_resize(proc,
                                  zebra_image_get_width(img),
                                  zebra_image_get_height(img));
    if(!rc) {
        zebra_image_scanner_enable_cache(proc->scanner, 0);
        rc = process_image(proc, img);
        if(proc->active)
            zebra_image_scanner_enable_cache(proc->scanner, 1);
    }
    proc_unlock(proc);
    return(rc);
}
