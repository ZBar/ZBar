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

#include "window.h"

static int ximage_cleanup (zebra_window_t *w)
{
    if(w->img.x)
        free(w->img.x);
    w->img.x = NULL;
    return(0);
}

static inline int ximage_init (zebra_window_t *w,
                               zebra_image_t *img)
{
    if(w->img.x)
        free(w->img.x);
    if(w->img_format != img->format) {
        /* FIXME supported formats */
        /*_zebra_best_format(img->format, &w->format, w->formats);*/
        w->format = fourcc('R','G','B','3');
        if(!w->format) {
            err_capture_int(w, SEV_ERROR, ZEBRA_ERR_UNSUPPORTED, __func__,
                            "no conversion from %08x to supported formats",
                            img->format);
            return(-1);
        }
        w->img_format = img->format;
    }
    XImage *ximg = w->img.x = calloc(1, sizeof(XImage));
    ximg->width = img->width;
    ximg->height = img->height;
    ximg->format = ZPixmap;
    ximg->byte_order = LSBFirst;
    ximg->bitmap_unit = 8;
    ximg->bitmap_bit_order = MSBFirst;
    ximg->bitmap_pad = 8;
    ximg->depth = ximg->bits_per_pixel = 24;
    ximg->red_mask = 0x00ff0000L;
    ximg->green_mask = 0x0000ff00L;
    ximg->blue_mask = 0x000000ffL;
    XInitImage(ximg);
    return(0);
}

static int ximage_draw (zebra_window_t *w,
                        zebra_image_t *img)
{
    XImage *ximg = w->img.x;
    if(!ximg ||
       w->img_format != img->format ||
       ximg->width != w->width || ximg->height != w->height ||
       ximg->width != img->width || ximg->height != img->height) {
        if(ximage_init(w, img))
            return(-1);
        ximg = w->img.x;
    }
    if(img->format != w->format) {
        /* save *converted* image for redraw */
        w->image = zebra_image_convert(img, w->format);
        zebra_image_destroy(img);
        img = w->image;
    }

    ximg->data = (void*)img->data;
    /* FIXME implement some basic scaling */
    unsigned width = img->width;
    unsigned src_x = 0, dst_x = 0;
    if(w->width < img->width) {
        width = w->width;
        src_x = (img->width - w->width) >> 1;
    }
    else
        dst_x = (w->width - img->width) >> 1;

    unsigned height = img->height;
    unsigned src_y = 0, dst_y = 0;
    if(w->height < img->height) {
        height = w->height;
        src_y = (img->height - w->height) >> 1;
    }
    else
        dst_y = (w->height - img->height) >> 1;

    int screen = DefaultScreen(w->display);
    GC gc = DefaultGC(w->display, screen);
    XPutImage(w->display, w->xwin, gc, ximg,
              src_x, src_y, dst_x, dst_y, width, height);
    ximg->data = NULL;
    return(0);
}

int _zebra_window_probe_ximage (zebra_window_t *w)
{
    /* FIXME determine supported formats/depths */
    w->draw_image = ximage_draw;
    w->cleanup = ximage_cleanup;
    return(0);
}
