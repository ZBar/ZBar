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

/* unpack size/location of component */
#define RGB_SIZE(c)   ((c) >> 5)
#define RGB_OFFSET(c) ((c) & 0x1f)

/* coarse image format categorization.
 * to limit conversion variations
 */
typedef enum zebra_format_group_e {
    ZEBRA_FMT_GRAY,
    ZEBRA_FMT_YUV_PLANAR,
    ZEBRA_FMT_YUV_PACKED,
    ZEBRA_FMT_RGB_PACKED,
    ZEBRA_FMT_YUV_NV,
} zebra_format_group_t;


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

/* description of an image format */
typedef struct zebra_format_def_s {
    uint32_t format;                    /* fourcc */
    zebra_format_group_t group;         /* coarse categorization */
    union {
        uint8_t gen[4];                 /* raw bytes */
        struct {
            uint8_t bpp;                /* bits per pixel */
            uint8_t red, green, blue;   /* size/location a la RGB_BITS() */
        } rgb;
        struct {
            uint8_t xsub2, ysub2;       /* chroma subsampling in each axis */
            uint8_t packorder;          /* channel ordering flags
                                         *   bit0: 0=UV, 1=VU
                                         *   bit1: 0=Y/chroma, 1=chroma/Y
                                         */
        } yuv;
        uint32_t cmp;                   /* quick compare equivalent formats */
    } p;
} zebra_format_def_t;

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

extern int _zebra_best_format(uint32_t, uint32_t*, const uint32_t*);
extern const zebra_format_def_t *_zebra_format_lookup(uint32_t);

#endif
