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
#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <config.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <stdlib.h>
#include <assert.h>

#include <zebra.h>
#include "symbol.h"

/* adapted from v4l2 spec */
#define fourcc(a, b, c, d)                      \
    ((uint32_t)(a) | ((uint32_t)(b) << 8) |     \
     ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))


struct zebra_image_s {
    uint32_t format;            /* fourcc image format code */
    unsigned width, height;     /* image size */
    const void *data;           /* image sample data */
    size_t datalen;             /* allocated/mapped size of data */

    /* cleanup handler */
    zebra_image_cleanup_handler_t *cleanup;
    int refcnt;                 /* reference count */
    zebra_video_t *src;         /* originator */
    int srcidx;                 /* index used by originator */
    zebra_image_t *next;        /* internal image lists */

    int nsyms;                  /* number of valid symbols */
    zebra_symbol_t *syms;       /* first of decoded symbol results */
};

static inline void _zebra_image_refcnt (zebra_image_t *img,
                                        int delta)
{
    img->refcnt += delta;
    assert(img->refcnt >= 0);
    if(!img->refcnt) {
        if(img->cleanup)
            img->cleanup(img);
        if(!img->src)
            free(img);
    }
}

int _zebra_best_format(uint32_t, uint32_t*, const uint32_t*);


#endif
