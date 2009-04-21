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
#ifndef _PROCESSOR_H_
#define _PROCESSOR_H_

#include <config.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

#ifdef HAVE_X
# include <X11/Xlib.h>
#endif

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#include <zbar.h>
#include "error.h"

typedef int (poll_handler_t)(zbar_processor_t*, int);

/* poll information */
typedef struct poll_desc_s {
    int num;                            /* number of descriptors */
    struct pollfd *fds;                 /* poll descriptors */
    poll_handler_t **handlers;          /* poll handlers */
} poll_desc_t;

struct zbar_processor_s {
    errinfo_t err;                      /* error reporting */
    const void *userdata;               /* application data */

    zbar_video_t *video;                /* input video device abstraction */
    zbar_window_t *window;              /* output window abstraction */
    zbar_image_scanner_t *scanner;      /* barcode scanner */

    zbar_image_data_handler_t *handler; /* application data handler */

    unsigned req_width, req_height;     /* application requested video size */
    int req_v4l, req_iomode;            /* application requested v4l version */
    uint32_t force_input;               /* force input format (debug) */
    uint32_t force_output;              /* force format conversion (debug) */

    /* state flags */
    unsigned threaded      : 1;         /* threading requested */
    unsigned visible       : 1;         /* output window mapped to display */
    volatile unsigned active : 1;       /* async processor thread running */

    volatile unsigned events;           /* synchronization events */
#define EVENT_INPUT     0x01            /* user input */
#define EVENT_OUTPUT    0x02            /* decoded output data available */

    int input;                          /* user input status */

    volatile poll_desc_t polling;       /* polling registration */
    poll_desc_t thr_polling;            /* thread copy */
    int kick_fds[2];                    /* poll kicker */

#ifdef HAVE_LIBPTHREAD
    int sem;                            /* window access semaphore */
    pthread_t sem_owner;                /* semaphore owner */
    pthread_mutex_t mutex;              /* semaphore lock */
    pthread_cond_t cond;                /* semaphore condition */
    pthread_cond_t event;
    pthread_t video_thread;             /* video input handler */
    pthread_t input_thread;             /* window event handler */
#endif
    unsigned video_started;             /* thread active flags */
    unsigned input_started;

#ifdef HAVE_X
    Display *display;                   /* X display connection */
    Window xwin;                        /* toplevel window */
#else
    void *display;                      /* generic placeholder */
    unsigned long xwin;                 /* unused */
#endif
};

static inline int alloc_polls (volatile poll_desc_t *p)
{
    p->fds = realloc(p->fds, p->num * sizeof(struct pollfd));
    p->handlers = realloc(p->handlers, p->num * sizeof(poll_handler_t*));
    /* FIXME should check for ENOMEM */
    return(0);
}

static inline int add_poll (zbar_processor_t *proc,
                            int fd,
                            poll_handler_t *handler)
{
    unsigned i = proc->polling.num++;
    zprintf(5, "[%d] fd=%d handler=%p\n", i, fd, handler);
    if(alloc_polls(&proc->polling))
        return(-1);
    memset(&proc->polling.fds[i], 0, sizeof(struct pollfd));
    proc->polling.fds[i].fd = fd;
    proc->polling.fds[i].events = POLLIN;
    proc->polling.handlers[i] = handler;
#ifdef HAVE_LIBPTHREAD
    if(proc->input_started) {
        assert(proc->kick_fds[1] >= 0);
        write(proc->kick_fds[1], &i /* unused */, sizeof(unsigned));
        /* FIXME should sync */
    }
#endif
    return(i);
}

static inline int remove_poll (zbar_processor_t *proc,
                               int fd)
{
    int i;
    for(i = proc->polling.num - 1; i >= 0; i--)
        if(proc->polling.fds[i].fd == fd)
            break;
    zprintf(5, "[%d] fd=%d n=%d\n", i, fd, proc->polling.num);
    if(i < 0)
        return(1);
    if(i + 1 < proc->polling.num) {
        int n = proc->polling.num - i - 1;
        memmove(&proc->polling.fds[i], &proc->polling.fds[i + 1],
                n * sizeof(struct pollfd));
        memmove(&proc->polling.handlers[i], &proc->polling.handlers[i + 1],
                n * sizeof(poll_handler_t));
    }
    proc->polling.num--;
    int rc = alloc_polls(&proc->polling);
#ifdef HAVE_LIBPTHREAD
    if(proc->input_started) {
        write(proc->kick_fds[1], &i /* unused */, sizeof(unsigned));
        /* FIXME should sync */
    }
#endif
    return(rc);
}

static inline void emit (zbar_processor_t *proc,
                         unsigned mask)
{
    proc->events |= mask;
#ifdef HAVE_LIBPTHREAD
    if(!proc->threaded)
        return;
    pthread_cond_broadcast(&proc->event);
#endif
}

extern int _zbar_window_open(zbar_processor_t*, char*, unsigned, unsigned);
extern int _zbar_window_close(zbar_processor_t*);
extern int _zbar_window_set_visible(zbar_processor_t*, int);
extern int _zbar_window_invalidate(zbar_window_t*);
extern int _zbar_window_set_size(zbar_processor_t*, unsigned, unsigned);
extern int _zbar_window_handle_events(zbar_processor_t*, int);

#endif
