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

#include <zebra.h>
#include "image.h"
#include "error.h"

struct zebra_window_s {
    errinfo_t err;              /* error reporting */
    zebra_image_t *image;       /* last displayed image */
    unsigned overlay;           /* user set overlay level */

    uint32_t format;            /* output format */
    unsigned width, height;     /* current output size */
    unsigned max_width, max_height;

    uint32_t *formats;          /* supported formats (zero terminated) */

    /* interface dependent methods */
    int (*draw_image)(zebra_window_t*, zebra_image_t*);
    int (*cleanup)(zebra_window_t*);

#ifdef HAVE_X
    Display *display;           /* display connection */
    Drawable xwin;              /* platform window handle */
    union {
        XImage *x;
#ifdef HAVE_X11_EXTENSIONS_XVLIB_H
        XvImage *xv;
#endif
    } img;
    uint32_t img_format;        /* current input format */
    XID img_port;               /* current format port */

    XID *xv_ports;              /* best port for format */
    int num_xv_adaptors;        /* number of adaptors */
    XID *xv_adaptors;           /* port grabbed for each adaptor */

# ifdef HAVE_X11_EXTENSIONS_XSHM_H
    XShmSegmentInfo shm;        /* shared memory segment */
# endif

    unsigned long colors[8];    /* pre-allocated colors */
#endif
};

static inline int _zebra_window_add_format (zebra_window_t *w,
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

extern int _zebra_window_probe_ximage(zebra_window_t*);
extern int _zebra_window_probe_xshm(zebra_window_t*);
extern int _zebra_window_probe_xv(zebra_window_t*);

extern int _zebra_window_attach(zebra_window_t*, void*, unsigned long);

extern int _zebra_window_clear(zebra_window_t*);

extern int _zebra_window_draw_marker(zebra_window_t*, uint32_t,
                                     const point_t*);
#if 0
extern int _zebra_window_draw_line(zebra_window_t*, uint32_t,
                                   const point_t*, const point_t*);
extern int _zebra_window_draw_outline(zebra_window_t*, uint32_t,
                                      const symbol_t*);
extern int _zebra_window_draw_text(zebra_window_t*, uint32_t,
                                   const point_t*, const char*);
#endif

#endif
