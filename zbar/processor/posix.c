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
#include "posix.h"
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
inline int _zbar_processor_lock (const zbar_processor_t *proc)
{
    if(!proc->threaded)
        return(0);

    processor_state_t *state = proc->state;
    pthread_t self = pthread_self();

    int rc = pthread_mutex_lock(&state->mutex);
    pthread_cleanup_push(proc_mutex_unlock, &state->mutex);

    if(!rc && state->lock_level && pthread_equal(state->lock_owner, self))
        state->lock_level++;
    else {
        while(!rc && state->lock_level)
            rc = pthread_cond_wait(&state->cond, &state->mutex);
        if(!rc) {
            state->lock_level++;
            state->lock_owner = self;
        }
    }

    pthread_cleanup_pop(1);

    if(rc)
        err_capture(proc, SEV_ERROR, ZBAR_ERR_LOCKING, __func__,
                    "unable to lock processor");
    return(rc);
}

/* release window access semaphore */
inline int _zbar_processor_unlock (const zbar_processor_t *proc)
{
    if(!proc->threaded)
        return(0);
    processor_state_t *state = proc->state;

    pthread_mutex_lock(&state->mutex);

    assert(state->lock_level > 0);
    assert(pthread_equal(state->lock_owner, pthread_self()));
    if(!--state->lock_level)
        pthread_cond_signal(&state->cond);

    pthread_mutex_unlock(&state->mutex);

    return(0);
}

inline int _zbar_processor_notify (zbar_processor_t *proc,
                                   unsigned event)
{
    proc->events |= event;
#ifdef HAVE_LIBPTHREAD
    processor_state_t *state = proc->state;
    pthread_cond_broadcast(&state->event);
#endif
    return(0);
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
# define _zbar_processor_lock(...) (0)
# define _zbar_processor_unlock(...) (0)
# define proc_destroy_thread(...)
#endif


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
    int rc = _zbar_process_image(proc, img);
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
    processor_state_t *state = proc->state;
    _zbar_processor_lock(proc);
    if(p->fds[0].revents && p->fds[0].fd == state->kick_fds[0]) {
        unsigned junk[2];
        read(state->kick_fds[0], junk, 2 * sizeof(unsigned));
        _zbar_processor_unlock(proc);
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
    _zbar_processor_unlock(proc);
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
static inline int proc_wait_threaded (zbar_processor_t *proc,
                                      unsigned event,
                                      int timeout,
                                      struct timespec *abstime)
{
    processor_state_t *state = proc->state;
    pthread_t self = pthread_self();

    int rc = pthread_mutex_lock(&state->mutex);

    assert(state->lock_level > 0);
    assert(pthread_equal(state->lock_owner, self));
    proc->events &= ~event;

    int save_level = state->lock_level;
    state->lock_level = 0;

    pthread_cond_signal(&state->cond);

    pthread_cleanup_push(proc_mutex_unlock, &state->mutex);
    while(!rc && !(proc->events & event))
        if(timeout < 0)
            rc = pthread_cond_wait(&state->event, &state->mutex);
        else
            rc = pthread_cond_timedwait(&state->event, &state->mutex, abstime);
    if(rc == ETIMEDOUT)
        rc = 0;

    while(!rc && state->lock_level)
        rc = pthread_cond_wait(&state->cond, &state->mutex);
    if(!rc) {
        state->lock_level = save_level;
        state->lock_owner = self;
    }

    pthread_cleanup_pop(1);

    if(rc)
        return(-1); /* error (FIXME save info) */
    if(proc->events & event)
        return(1); /* got event */
    return(0); /* timed out */
}
#endif

static inline int proc_wait_unthreaded (zbar_processor_t *proc,
                                        unsigned event,
                                        int timeout,
                                        struct timespec *abstime)
{
    processor_state_t *state = proc->state;
    int blocking = proc->active && zbar_video_get_fd(proc->video) < 0;
    proc->events &= ~event;
    /* unthreaded, poll here for window input */
    while(!(proc->events & event)) {
        if(blocking) {
            zbar_image_t *img = zbar_video_next_image(proc->video);
            if(!img)
                return(-1);
            _zbar_process_image(proc, img);
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
        if(state->polling.num)
            proc_poll_inputs(proc, (poll_desc_t*)&state->polling, reltime);
        else if(!blocking) {
            _zbar_processor_unlock(proc);
            struct timespec sleepns, remns;
            sleepns.tv_sec = timeout / 1000;
            sleepns.tv_nsec = (timeout % 1000) * 1000000;
            while(nanosleep(&sleepns, &remns) && errno == EINTR)
                sleepns = remns;
            (void)_zbar_processor_lock(proc);
            return(0);
        }
    }
    return(1);
}

inline int _zbar_processor_wait (zbar_processor_t *proc,
                                 unsigned event,
                                 int timeout)
{
    struct timespec abstime;
    proc_calc_abstime(&abstime, timeout);
#ifdef HAVE_LIBPTHREAD
    if(proc->threaded)
        return(proc_wait_threaded(proc, event, timeout, &abstime));
    else
#endif
        return(proc_wait_unthreaded(proc, event, timeout, &abstime));
}

#ifdef HAVE_LIBPTHREAD
/* make a thread-local copy of polling data */
static inline void proc_cache_polling (zbar_processor_t *proc)
{
    (void)_zbar_processor_lock(proc);
    processor_state_t *state = proc->state;
    int n = state->polling.num;
    zprintf(5, "%d fds\n", n);
    state->thr_polling.num = n;
    alloc_polls(&state->thr_polling);
    memcpy(state->thr_polling.fds, state->polling.fds,
           n * sizeof(struct pollfd));
    memcpy(state->thr_polling.handlers, state->polling.handlers,
           n * sizeof(poll_handler_t*));
    _zbar_processor_unlock(proc);
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
    processor_state_t *state = proc->state;
    proc_block_sigs();

    /* wait for registered inputs and process using associated handler */
    while(1) {
        proc_cache_polling(proc);

        int rc = 0;
        while(!rc) {
            if(state->polling.num)
                rc = proc_poll_inputs(proc, &state->thr_polling, -1);
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
    processor_state_t *state = proc->state;
    pthread_t self = pthread_self();
    proc_block_sigs();

    (void)_zbar_processor_lock(proc);
    while(1) {
        /* wait for active to be set */
        assert(state->lock_level == 1);
        assert(pthread_equal(state->lock_owner, self));
        pthread_mutex_lock(&state->mutex);
        state->lock_level = 0;
        pthread_cond_signal(&state->cond);
        pthread_cleanup_push(proc_mutex_unlock, &state->mutex);
        while(!proc->active)
            pthread_cond_wait(&state->event, &state->mutex);
        pthread_cleanup_pop(1);

        /* unlocked blocking image input */
        zbar_image_t *img = zbar_video_next_image(proc->video);
        if(!img)
            return(NULL);
        (void)_zbar_processor_lock(proc);
        if(proc->active)
            _zbar_process_image(proc, img);
        zbar_image_destroy(img);
    }
}
#endif

int _zbar_processor_threads_init (zbar_processor_t *proc,
                                  int threaded)
{
    processor_state_t *state = proc->state =
        calloc(1, sizeof(processor_state_t));
    state->kick_fds[0] = state->kick_fds[1] = -1;

    if(threaded) {
#ifdef HAVE_LIBPTHREAD
        /* FIXME check errors */
        pthread_mutex_init(&state->mutex, NULL);
        pthread_cond_init(&state->cond, NULL);
        pthread_cond_init(&state->event, NULL);
        pipe(state->kick_fds);
        add_poll(state, state->kick_fds[0], NULL);
        proc->threaded = 1;
#else
    /* FIXME record warning */
#endif
    }
    return(0);
}

int _zbar_processor_threads_start (zbar_processor_t *proc)
{
    if(!proc->threaded)
        return(0);

#ifdef HAVE_LIBPTHREAD
    processor_state_t *state = proc->state;
    if(proc->video && zbar_video_get_fd(proc->video) < 0) {
        /* spawn blocking video thread */
        int rc;
        if((rc = pthread_create(&state->video_thread, NULL,
                                proc_video_thread, proc)))
            return(err_capture_num(proc, SEV_ERROR, ZBAR_ERR_SYSTEM,
                                   __func__, "spawning video thread", rc));

        state->video_started = 1;
        zprintf(4, "spawned video thread\n");
    }

    if(proc->window ||
       (proc->video && !state->video_started)) {
        /* spawn input monitor thread */
        int rc;
        if((rc = pthread_create(&state->input_thread, NULL,
                                proc_input_thread, proc)))
            return(err_capture_num(proc, SEV_ERROR, ZBAR_ERR_SYSTEM, __func__,
                                   "spawning input thread", rc));
        state->input_started = 1;
        zprintf(4, "spawned input thread\n");
    }
#endif

    return(0);
}

int _zbar_processor_threads_stop (zbar_processor_t *proc)
{
#ifdef HAVE_LIBPTHREAD
    processor_state_t *state = proc->state;
    proc_destroy_thread(state->video_thread, &state->video_started);
    proc_destroy_thread(state->input_thread, &state->input_started);
#endif
    return(0);
}

int _zbar_processor_threads_cleanup (zbar_processor_t *proc)
{
    processor_state_t *state = proc->state;
#ifdef HAVE_LIBPTHREAD
    if(proc->threaded) {
        pthread_cond_destroy(&state->event);
        pthread_cond_destroy(&state->cond);
        pthread_mutex_destroy(&state->mutex);
    }
#endif
    if(state->polling.fds) {
        free(state->polling.fds);
        state->polling.fds = NULL;
    }
    if(state->polling.handlers) {
        free(state->polling.handlers);
        state->polling.handlers = NULL;
    }
    if(state->thr_polling.fds) {
        free(state->thr_polling.fds);
        state->thr_polling.fds = NULL;
    }
    if(state->thr_polling.handlers) {
        free(state->thr_polling.handlers);
        state->thr_polling.handlers = NULL;
    }
    free(proc->state);
    proc->state = NULL;
    return(0);
}

int _zbar_processor_enable (zbar_processor_t *proc,
                            int active)
{
    int vid_fd = zbar_video_get_fd(proc->video);
    processor_state_t *state = proc->state;
    if(vid_fd >= 0) {
        if(active)
            add_poll(state, vid_fd, proc_video_handler);
        else
            remove_poll(state, vid_fd);
    }
    /* FIXME failure recovery? */
    return(0);
}
