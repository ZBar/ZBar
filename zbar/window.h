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
#ifndef _WINDOW_H_
#define _WINDOW_H_

#include <config.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <stdlib.h>

#ifdef HAVE_X
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# ifdef HAVE_X11_EXTENSIONS_XSHM_H
#  include <X11/extensions/XShm.h>
# endif
#ifdef HAVE_X11_EXTENSIONS_XVLIB_H
#  include <X11/extensions/Xvlib.h>
#endif
#endif

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#include <zbar.h>
#include "image.h"
#include "error.h"

struct zbar_window_s {
    errinfo_t err;              /* error reporting */
    zbar_image_t *image;        /* last displayed image
                                 * NB image access must be locked!
                                 */
    unsigned overlay;           /* user set overlay level */

    uint32_t format;            /* output format */
    unsigned width, height;     /* current output size */
    unsigned max_width, max_height;

    uint32_t *formats;          /* supported formats (zero terminated) */

    /* interface dependent methods */
    int (*draw_image)(zbar_window_t*, zbar_image_t*);
    int (*cleanup)(zbar_window_t*);

#ifdef HAVE_X
    Display *display;           /* display connection */
    Drawable xwin;              /* platform window handle */
    GC gc;                      /* graphics context */
    union {
        XImage *x;
#ifdef HAVE_X11_EXTENSIONS_XVLIB_H
        XvImage *xv;
#endif
    } img;
    uint32_t src_format;        /* current input format */
    unsigned src_width;         /* last displayed image size */
    unsigned src_height;
    unsigned dst_width;         /* conversion target */
    XID img_port;               /* current format port */

    XID *xv_ports;              /* best port for format */
    int num_xv_adaptors;        /* number of adaptors */
    XID *xv_adaptors;           /* port grabbed for each adaptor */

# ifdef HAVE_X11_EXTENSIONS_XSHM_H
    XShmSegmentInfo shm;        /* shared memory segment */
# endif

    unsigned long colors[8];    /* pre-allocated colors */

    Region exposed;

    /* pre-calculated logo geometries */
    int logo_scale;
    unsigned long logo_colors[2];
    Region logo_zbars;
    XPoint logo_z[4];
    XRectangle logo_bars[5];
#endif

#ifdef HAVE_LIBPTHREAD
    pthread_mutex_t imglock;    /* lock displayed image */
#endif
};


#ifdef HAVE_LIBPTHREAD

/* window.draw has to be thread safe wrt/other apis
 * FIXME should be a semaphore
 */
static inline int window_lock (zbar_window_t *w)
{
    int rc = 0;
    if((rc = pthread_mutex_lock(&w->imglock))) {
        err_capture(w, SEV_FATAL, ZBAR_ERR_LOCKING, __func__,
                    "unable to acquire lock");
        w->err.errnum = rc;
        return(-1);
    }
    return(0);
}

static inline int window_unlock (zbar_window_t *w)
{
    int rc = 0;
    if((rc = pthread_mutex_unlock(&w->imglock))) {
        err_capture(w, SEV_FATAL, ZBAR_ERR_LOCKING, __func__,
                    "unable to release lock");
        w->err.errnum = rc;
        return(-1);
    }
    return(0);
}

#else
# define window_lock(...) (0)
# define window_unlock(...) (0)
#endif

static inline int _zbar_window_add_format (zbar_window_t *w,
                                            uint32_t fmt)
{
    int i;
    for(i = 0; w->formats && w->formats[i]; i++)
        if(w->formats[i] == fmt)
            return(i);

    w->formats = realloc(w->formats, (i + 2) * sizeof(uint32_t));
    w->formats[i] = fmt;
    w->formats[i + 1] = 0;
    return(i);
}

extern int _zbar_window_probe_ximage(zbar_window_t*);
extern int _zbar_window_probe_xshm(zbar_window_t*);
extern int _zbar_window_probe_xv(zbar_window_t*);

extern int _zbar_window_attach(zbar_window_t*,
                               void*,
                               unsigned long);
extern int _zbar_window_resize(zbar_window_t *w);

extern int _zbar_window_clear(zbar_window_t*);

extern int _zbar_window_draw_marker(zbar_window_t*, uint32_t,
                                    const point_t*);
#if 0
extern int _zbar_window_draw_line(zbar_window_t*, uint32_t,
                                  const point_t*, const point_t*);
extern int _zbar_window_draw_outline(zbar_window_t*, uint32_t,
                                     const symbol_t*);
extern int _zbar_window_draw_text(zbar_window_t*, uint32_t,
                                  const point_t*, const char*);
#endif

#endif
