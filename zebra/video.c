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

#include "video.h"
#include "image.h"

static void _zebra_video_recycle_image (zebra_image_t *img)
{
    zebra_video_t *vdo = img->src;
    assert(vdo);
    assert(img->srcidx >= 0);
    video_lock(vdo);
    if(vdo->images[img->srcidx] != img)
        vdo->images[img->srcidx] = img;
    if(vdo->active)
        vdo->nq(vdo, img);
    else
        video_unlock(vdo);
}

zebra_video_t *zebra_video_create ()
{
    zebra_video_t *vdo = calloc(1, sizeof(zebra_video_t));
    if(!vdo)
        return(NULL);
    err_init(&vdo->err, ZEBRA_MOD_VIDEO);
    vdo->fd = -1;

#ifdef HAVE_PTHREAD_H
    if(pthread_mutex_init(&vdo->qlock, NULL)) {
        free(vdo);
        return(NULL);
    }
#endif

    /* pre-allocate images */
    vdo->num_images = ZEBRA_VIDEO_IMAGES_MAX;
    vdo->images = calloc(vdo->num_images, sizeof(zebra_image_t*));
    if(!vdo->images) {
        zebra_video_destroy(vdo);
        return(NULL);
    }

    int i;
    for(i = 0; i < vdo->num_images; i++) {
        zebra_image_t *img = vdo->images[i] = zebra_image_create();
        if(!img) {
            zebra_video_destroy(vdo);
            return(NULL);
        }
        img->refcnt = 0;
        img->cleanup = _zebra_video_recycle_image;
        img->srcidx = i;
        img->src = vdo;
    }

    return(vdo);
}

void zebra_video_destroy (zebra_video_t *vdo)
{
    if(vdo->fd >= 0)
        zebra_video_open(vdo, NULL);
    if(vdo->images) {
        int i;
        for(i = 0; i < vdo->num_images; i++)
            if(vdo->images[i])
                free(vdo->images[i]);
        free(vdo->images);
    }
    if(vdo->buf)
        free(vdo->buf);
    if(vdo->formats)
        free(vdo->formats);
    err_cleanup(&vdo->err);
#ifdef HAVE_PTHREAD_H
    pthread_mutex_destroy(&vdo->qlock);
#endif
    free(vdo);
}

int zebra_video_open (zebra_video_t *vdo,
                      const char *dev)
{
#if !defined(HAVE_LINUX_VIDEODEV_H)
    err_capture(vdo, SEV_ERROR, ZEBRA_ERR_UNSUPPORTED, __func__,
                "not compiled with video support");
    _zebra_spew_error(vdo);
    return(-1);
#else
    return(_zebra_video_open(vdo, dev));
#endif    
}

int zebra_video_get_fd (const zebra_video_t *vdo)
{
    if(vdo->fd >= 0 && vdo->intf == VIDEO_V4L2)
        return(vdo->fd);

    if(vdo->intf != VIDEO_V4L2)
        return(err_capture(vdo, SEV_WARNING, ZEBRA_ERR_UNSUPPORTED, __func__,
                           "v4l1 API does not support polling"));

    return(err_capture(vdo, SEV_ERROR, ZEBRA_ERR_INVALID, __func__,
                       "video device not opened"));
}

int zebra_video_get_width (const zebra_video_t *vdo)
{
    return(vdo->width);
}

int zebra_video_get_height (const zebra_video_t *vdo)
{
    return(vdo->height);
}

static inline int video_init_images (zebra_video_t *vdo)
{
    
    assert(vdo->datalen);
    if(vdo->iomode != VIDEO_MMAP) {
        assert(!vdo->buf);
        vdo->buflen = vdo->num_images * vdo->datalen;
        vdo->buf = malloc(vdo->buflen);
        if(!vdo->buf)
            return(err_capture(vdo, SEV_FATAL, ZEBRA_ERR_NOMEM, __func__,
                               "unable to allocate image buffers"));
        zprintf(1, "pre-allocated %d %s buffers size=0x%x\n", vdo->num_images,
                (vdo->iomode == VIDEO_READWRITE) ? "READ" : "USERPTR",
                vdo->buflen);
    }
    int i;
    for(i = 0; i < vdo->num_images; i++) {
        zebra_image_t *img = vdo->images[i];
        img->format = vdo->format;
        img->width = vdo->width;
        img->height = vdo->height;
        if(vdo->iomode != VIDEO_MMAP) {
            img->datalen = vdo->datalen;
            size_t offset = i * vdo->datalen;
            img->data = vdo->buf + offset;
            zprintf(2, "    [%02d] @%08x\n", i, offset);
        }
        else {
            assert(img->data);
            assert(img->datalen);
            assert(img->datalen >= vdo->datalen);
        }
    }
    return(0);
}

int _zebra_video_init (zebra_video_t *vdo,
                       uint32_t fmt)
{
    if(vdo->initialized)
        /* FIXME re-init different format? (and export api) */
        return(err_capture(vdo, SEV_ERROR, ZEBRA_ERR_INVALID, __func__,
                           "already initialized, can't re-init"));

    if(vdo->init(vdo, fmt))
        return(-1);
    vdo->format = fmt;
    if(video_init_images(vdo))
        return(-1);
    vdo->initialized = 1;
    return(0);
}

int zebra_video_enable (zebra_video_t *vdo,
                        int enable)
{
    if(vdo->active == enable)
        return(0);

    if(enable) {
        if(vdo->fd < 0)
            return(err_capture(vdo, SEV_ERROR, ZEBRA_ERR_INVALID, __func__,
                               "video device not opened"));

        if(!vdo->initialized &&
           zebra_negotiate_format(vdo, NULL))
            return(-1);
    }

    if(video_lock(vdo))
        return(-1);
    vdo->active = enable;
    if(enable)
        return(vdo->start(vdo));
    else
        return(vdo->stop(vdo));
}

zebra_image_t *zebra_video_next_image (zebra_video_t *vdo)
{
    if(video_lock(vdo))
        return(NULL);
    if(!vdo->active) {
            err_capture(vdo, SEV_ERROR, ZEBRA_ERR_INVALID, __func__,
                        "video not enabled");
        video_unlock(vdo);
        return(NULL);
    }
    zebra_image_t *img = vdo->dq(vdo);
    if(img)
        img->refcnt++;
    return(img);
}
