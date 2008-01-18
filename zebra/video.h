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
#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <config.h>

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifdef HAVE_PTHREAD_H
# include <pthread.h>
#endif

#include <zebra.h>

#include "error.h"

/* number of images to preallocate */
#define ZEBRA_VIDEO_IMAGES_MAX  4

typedef enum video_interface_e {
    VIDEO_INVALID = 0,          /* uninitialized */
    VIDEO_V4L1,                 /* v4l protocol version 1 */
    VIDEO_V4L2,                 /* v4l protocol version 2 */
} video_interface_t;

typedef enum video_iomode_e {
    VIDEO_READWRITE = 1,        /* standard system calls */
    VIDEO_MMAP,                 /* mmap interface */
    VIDEO_USERPTR,              /* userspace buffers */
} video_iomode_t;

struct zebra_video_s {
    errinfo_t err;              /* error reporting */
    int fd;                     /* open camera device */
    unsigned width, height;     /* video frame size */

    video_interface_t intf;     /* input interface type */
    video_iomode_t iomode;      /* video data transfer mode */
    unsigned initialized : 1;   /* format selected and images mapped */
    unsigned active      : 1;   /* current streaming state */

    uint32_t format;            /* selected fourcc */
    unsigned palette;           /* v4l1 format index corresponding to format */
    uint32_t *formats;          /* 0 terminated list of supported formats */

    size_t datalen;             /* size of image data for selected format */
    size_t buflen;              /* total size of image data buffer */
    void *buf;                  /* image data buffer */

    int num_images;             /* number of allocated images */
    zebra_image_t **images;     /* indexed list of images */
    zebra_image_t *nq_image;    /* last image enqueued */
    zebra_image_t *dq_image;    /* first image to dequeue (when ordered) */

#ifdef HAVE_PTHREAD_H
    pthread_mutex_t qlock;      /* lock image queue */
#endif

    /* interface dependent methods */
    int (*init)(zebra_video_t*, uint32_t);
    int (*cleanup)(zebra_video_t*);
    int (*start)(zebra_video_t*);
    int (*stop)(zebra_video_t*);
    int (*nq)(zebra_video_t*, zebra_image_t*);
    zebra_image_t* (*dq)(zebra_video_t*);
};


#ifdef HAVE_PTHREAD_H

/* video.next_image and video.recycle_image have to be thread safe
 * wrt/other apis
 */
static inline int video_lock (zebra_video_t *vdo)
{
    int rc = 0;
    if((rc = pthread_mutex_lock(&vdo->qlock))) {
        err_capture(vdo, SEV_FATAL, ZEBRA_ERR_LOCKING, __func__,
                    "unable to acquire lock");
        vdo->err.errnum = rc;
        return(-1);
    }
    return(0);
}

static inline int video_unlock (zebra_video_t *vdo)
{
    int rc = 0;
    if((rc = pthread_mutex_unlock(&vdo->qlock))) {
        err_capture(vdo, SEV_FATAL, ZEBRA_ERR_LOCKING, __func__,
                    "unable to release lock");
        vdo->err.errnum = rc;
        return(-1);
    }
    return(0);
}

#else
# define video_lock(...) (0)
# define video_unlock(...) (0)
#endif

extern int _zebra_video_open(zebra_video_t*, const char*);
extern int _zebra_v4l2_probe(zebra_video_t*);

#endif
