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

#include "image.h"
#include "error.h"

zebra_image_t *zebra_image_create ()
{
    zebra_image_t *img = calloc(1, sizeof(zebra_image_t));
    img->refcnt = 1;
    img->srcidx = -1;
    return(img);
}

void zebra_image_destroy (zebra_image_t *img)
{
    _zebra_image_refcnt(img, -1);
}

unsigned long zebra_image_get_format (const zebra_image_t *img)
{
    return(img->format);
}

unsigned zebra_image_get_width (const zebra_image_t *img)
{
    return(img->width);
}

unsigned zebra_image_get_height (const zebra_image_t *img)
{
    return(img->height);
}

const void *zebra_image_get_data (const zebra_image_t *img)
{
    return(img->data);
}

void zebra_image_set_format (zebra_image_t *img,
                             unsigned long fmt)
{
    img->format = fmt;
}

void zebra_image_set_size (zebra_image_t *img,
                           unsigned w,
                           unsigned h)
{
    img->width = w;
    img->height = h;
}

inline void zebra_image_free_data (zebra_image_t *img)
{
    if(img->src) {
        /* replace video image w/new copy */
        assert(img->refcnt);
        zebra_image_t *newimg = zebra_image_create(img);
        memcpy(newimg, img, sizeof(zebra_image_t));
        /* recycle video image */
        newimg->cleanup(newimg);
        /* detach old image from src */
        img->cleanup = NULL;
        img->src = NULL;
        img->srcidx = -1;
    }
    else if(img->cleanup) {
        if(img->cleanup != zebra_image_free_data)
            img->cleanup(img);
        else if(img->data)
            free((void*)img->data);
    }
    img->data = NULL;
}

void zebra_image_set_data (zebra_image_t *img,
                           const void *data,
                           unsigned long len,
                           zebra_image_cleanup_handler_t *cleanup)
{
    zebra_image_free_data(img);
    img->data = data;
    img->datalen = len;
    img->cleanup = cleanup;
}

const zebra_symbol_t *zebra_image_first_symbol (const zebra_image_t *img)
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

int zebra_image_write (const zebra_image_t *img,
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
        n = snprintf(filename, len, "%s.%08x.zimg", filebase, img->format);
    assert(n < len);
    filename[len] = '\0';

    zprintf(1, "dumping %.4s(%08x) image to %s\n",
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
