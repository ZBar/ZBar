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
#ifndef _PROCESSOR_WIN_H_
#define _PROCESSOR_WIN_H_

#include <windows.h>

/* specific notification events */
typedef struct wait_entry_s {
    struct wait_entry_s *next;
    HANDLE notify;
    DWORD requester;
} wait_entry_t;

typedef struct wait_queue_s {
    wait_entry_t *head, *tail;
} wait_queue_t;

typedef struct thread_state_s {
    int running;
    HANDLE activity, started, stopped;
} thread_state_t;

struct processor_state_s {
    HWND hwnd;                          /* window handle */
    int input;
    unsigned width, height;

    CRITICAL_SECTION mutex;             /* shared data mutex */

    int lock_level;                     /* API serialization lock */
    DWORD lock_owner;
    wait_queue_t lock_queue;
    wait_queue_t wait_queue;
    wait_entry_t *free_entry;

    thread_state_t video_thread;
    thread_state_t input_thread;
};

static inline wait_entry_t *proc_entry_alloc (processor_state_t *state,
                                              DWORD self)
{
    wait_entry_t *entry = state->free_entry;
    if(entry)
        state->free_entry = entry->next;
    else {
        entry = malloc(sizeof(wait_entry_t));
        entry->notify = CreateEvent(NULL, 0, 0, NULL);
    }
    entry->next = NULL;
    entry->requester = self;
    return(entry);
}

static inline void proc_entry_free (processor_state_t *state,
                                    wait_entry_t *entry)
{
    if(entry) {
        entry->next = state->free_entry;
        state->free_entry = entry;
    }
}

static inline void proc_entry_queue (wait_queue_t *queue,
                                     wait_entry_t *entry)
{
    if(queue->head)
        queue->tail->next = entry;
    else
        queue->head = entry;
    queue->tail = entry;
}

static inline wait_entry_t *proc_entry_dequeue (wait_queue_t *queue)
{
    wait_entry_t *entry = queue->head;
    if(entry) {
        queue->head = entry->next;
        if(!entry->next)
            queue->tail = NULL;
        entry->next = NULL;
    }
    return(entry);
}

#endif
