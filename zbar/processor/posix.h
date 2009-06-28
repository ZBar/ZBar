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
#ifndef _PROCESSOR_POSIX_H_
#define _PROCESSOR_POSIX_H_

#include "processor.h"

#ifdef HAVE_POLL_H
# include <poll.h>
#endif

#ifdef HAVE_X
# include <X11/Xlib.h>
#endif

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#ifdef HAVE_POLL_H
typedef int (poll_handler_t)(zbar_processor_t*, int);

/* poll information */
typedef struct poll_desc_s {
    int num;                            /* number of descriptors */
    struct pollfd *fds;                 /* poll descriptors */
    poll_handler_t **handlers;          /* poll handlers */
} poll_desc_t;
#endif

struct processor_state_s {
#ifdef HAVE_POLL_H
    volatile poll_desc_t polling;       /* polling registration */
    poll_desc_t thr_polling;            /* thread copy */
    int kick_fds[2];                    /* poll kicker */
#endif

#ifdef HAVE_LIBPTHREAD
    int lock_level;                     /* API serialization lock */
    pthread_t lock_owner;               /* lock owner */
    pthread_mutex_t mutex;              /* lock mutex */
    pthread_cond_t cond;                /* lock notification condition */
    pthread_cond_t event;

    pthread_t video_thread;             /* video input handler */
    pthread_t input_thread;             /* window event handler */

    unsigned video_started;             /* thread active flags */
    unsigned input_started;
#endif
};


#ifdef HAVE_POLL_H
static inline int alloc_polls (volatile poll_desc_t *p)
{
    p->fds = realloc(p->fds, p->num * sizeof(struct pollfd));
    p->handlers = realloc(p->handlers, p->num * sizeof(poll_handler_t*));
    /* FIXME should check for ENOMEM */
    return(0);
}

static inline int add_poll (processor_state_t *state,
                            int fd,
                            poll_handler_t *handler)
{
    volatile poll_desc_t *polling = &state->polling;
    unsigned i = polling->num++;
    zprintf(5, "[%d] fd=%d handler=%p\n", i, fd, handler);
    if(alloc_polls(polling))
        return(-1);
    memset(&polling->fds[i], 0, sizeof(struct pollfd));
    polling->fds[i].fd = fd;
    polling->fds[i].events = POLLIN;
    polling->handlers[i] = handler;
#ifdef HAVE_LIBPTHREAD
    if(state->input_started) {
        assert(state->kick_fds[1] >= 0);
        write(state->kick_fds[1], &i /* unused */, sizeof(unsigned));
        /* FIXME should sync */
    }
#endif
    return(i);
}

static inline int remove_poll (processor_state_t *state,
                               int fd)
{
    volatile poll_desc_t *polling = &state->polling;
    int i;
    for(i = polling->num - 1; i >= 0; i--)
        if(polling->fds[i].fd == fd)
            break;
    zprintf(5, "[%d] fd=%d n=%d\n", i, fd, polling->num);
    if(i < 0)
        return(1);
    if(i + 1 < polling->num) {
        int n = polling->num - i - 1;
        memmove(&polling->fds[i], &polling->fds[i + 1],
                n * sizeof(struct pollfd));
        memmove(&polling->handlers[i], &polling->handlers[i + 1],
                n * sizeof(poll_handler_t));
    }
    polling->num--;
    int rc = alloc_polls(polling);
#ifdef HAVE_LIBPTHREAD
    if(state->input_started) {
        write(state->kick_fds[1], &i /* unused */, sizeof(unsigned));
        /* FIXME should sync */
    }
#endif
    return(rc);
}
#endif

#endif
