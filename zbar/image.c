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

#include "error.h"
#include "image.h"

zbar_image_t *zbar_image_create ()
{
    zbar_image_t *img = calloc(1, sizeof(zbar_image_t));
    img->refcnt = 1;
    img->srcidx = -1;
    return(img);
}

void zbar_image_destroy (zbar_image_t *img)
{
    _zbar_image_refcnt(img, -1);
}

void zbar_image_ref (zbar_image_t *img,
                     int refs)
{
    _zbar_image_refcnt(img, refs);
}

unsigned long zbar_image_get_format (const zbar_image_t *img)
{
    return(img->format);
}

unsigned zbar_image_get_sequence (const zbar_image_t *img)
{
    return(img->seq);
}

unsigned zbar_image_get_width (const zbar_image_t *img)
{
    return(img->width);
}

unsigned zbar_image_get_height (const zbar_image_t *img)
{
    return(img->height);
}

const void *zbar_image_get_data (const zbar_image_t *img)
{
    return(img->data);
}

unsigned long zbar_image_get_data_length (const zbar_image_t *img)
{
    return(img->datalen);
}

void zbar_image_set_format (zbar_image_t *img,
                            unsigned long fmt)
{
    img->format = fmt;
}

void zbar_image_set_sequence (zbar_image_t *img,
                              unsigned seq)
{
    img->seq = seq;
}

void zbar_image_set_size (zbar_image_t *img,
                          unsigned w,
                          unsigned h)
{
    img->width = w;
    img->height = h;
}

inline void zbar_image_free_data (zbar_image_t *img)
{
    if(!img)
        return;
    if(img->src) {
        /* replace video image w/new copy */
        assert(img->refcnt);
        zbar_image_t *newimg = zbar_image_create();
        memcpy(newimg, img, sizeof(zbar_image_t));
        /* recycle video image */
        newimg->cleanup(newimg);
        /* detach old image from src */
        img->cleanup = NULL;
        img->src = NULL;
        img->srcidx = -1;
    }
    else if(img->cleanup && img->data) {
        if(img->cleanup != zbar_image_free_data)
            img->cleanup(img);
        else
            free((void*)img->data);
    }
    img->data = NULL;
}

void zbar_image_set_data (zbar_image_t *img,
                          const void *data,
                          unsigned long len,
                          zbar_image_cleanup_handler_t *cleanup)
{
    zbar_image_free_data(img);
    img->data = data;
    img->datalen = len;
    img->cleanup = cleanup;
}

void zbar_image_set_userdata (zbar_image_t *img,
                              void *userdata)
{
    img->userdata = userdata;
}

void *zbar_image_get_userdata (const zbar_image_t *img)
{
    return(img->userdata);
}

zbar_image_t *zbar_image_copy (const zbar_image_t *src)
{
    zbar_image_t *dst = zbar_image_create();
    dst->format = src->format;
    dst->width = src->width;
    dst->height = src->height;
    dst->datalen = src->datalen;
    dst->data = malloc(src->datalen);
    assert(dst->data);
    memcpy((void*)dst->data, src->data, src->datalen);
    dst->cleanup = zbar_image_free_data;
    return(dst);
}

const zbar_symbol_t *zbar_image_first_symbol (const zbar_image_t *img)
{
    /* symbols stored on root image */
    while(img->next)
        img = img->next;
    return(img->syms);
}

typedef struct zimg_hdr_s {
    uint32_t magic, format;
    uint16_t width, height;
    uint32_t size;
} zimg_hdr_t;

int zbar_image_write (const zbar_image_t *img,
                      const char *filebase)
{
    int len = strlen(filebase) + 16;
    char filename[len];
    strcpy(filename, filebase);
    int n = 0;
    if(*(char*)&img->format >= ' ')
        n = snprintf(filename, len, "%s.%.4s.zimg",
                     filebase, (char*)&img->format);
    else
        n = snprintf(filename, len, "%s.%08" PRIx32 ".zimg",
                     filebase, img->format);
    assert(n < len);
    filename[len] = '\0';

    zprintf(1, "dumping %.4s(%08" PRIx32 ") image to %s\n",
            (char*)&img->format, img->format, filename);

    FILE *f = fopen(filename, "w");
    if(!f) {
        int rc = errno;
        zprintf(1, "ERROR opening %s: %s\n", filename, strerror(rc));
        return(rc);
    }

    zimg_hdr_t hdr;
    hdr.magic = 0x676d697a;
    hdr.format = img->format;
    hdr.width = img->width;
    hdr.height = img->height;
    hdr.size = img->datalen;

    if(fwrite(&hdr, sizeof(hdr), 1, f) != 1 ||
       fwrite(img->data, 1, img->datalen, f) != img->datalen) {
        int rc = errno;
        zprintf(1, "ERROR writing %s: %s\n", filename, strerror(rc));
        fclose(f);
        return(rc);
    }
    return(fclose(f));
}
