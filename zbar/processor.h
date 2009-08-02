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

#include <zbar.h>
#include "error.h"

/* platform specific state wrapper */
typedef struct processor_state_s processor_state_t;

struct zbar_processor_s {
    errinfo_t err;                      /* error reporting */
    const void *userdata;               /* application data */

    zbar_video_t *video;                /* input video device abstraction */
    zbar_window_t *window;              /* output window abstraction */
    zbar_image_scanner_t *scanner;      /* barcode scanner */

    zbar_image_data_handler_t *handler; /* application data handler */

    unsigned req_width, req_height;     /* application requested video size */
    int req_intf, req_iomode;           /* application requested interface */
    uint32_t force_input;               /* force input format (debug) */
    uint32_t force_output;              /* force format conversion (debug) */

    int input;                          /* user input status */

    /* state flags */
    int threaded;
    int visible;                        /* output window mapped to display */
    int active;                         /* async processor thread running */

    unsigned events;                    /* synchronization events */
#define EVENT_INPUT     0x01            /* user input */
#define EVENT_OUTPUT    0x02            /* decoded output data available */

    void *display;                      /* X display connection */
    unsigned long xwin;                 /* toplevel window */

    processor_state_t *state;
};

extern int _zbar_processor_lock(const zbar_processor_t*);
extern int _zbar_processor_unlock(const zbar_processor_t*);
extern int _zbar_processor_notify(zbar_processor_t*, unsigned);
extern int _zbar_processor_wait(zbar_processor_t*, unsigned, int);

extern int _zbar_processor_threads_init(zbar_processor_t*, int);
extern int _zbar_processor_threads_cleanup(zbar_processor_t*);
extern int _zbar_processor_threads_start(zbar_processor_t*,
                                         const char*);
extern int _zbar_processor_threads_stop(zbar_processor_t*);

extern int _zbar_processor_open(zbar_processor_t*, char*, unsigned, unsigned);
extern int _zbar_processor_close(zbar_processor_t*);
extern int _zbar_processor_set_visible(zbar_processor_t*, int);
extern int _zbar_processor_set_size(zbar_processor_t*, unsigned, unsigned);
extern int _zbar_processor_invalidate(zbar_processor_t*);
extern int _zbar_processor_enable(zbar_processor_t*, int);
extern int _zbar_process_image(zbar_processor_t*, zbar_image_t*);

#endif
