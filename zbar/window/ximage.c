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

#include "window.h"

static int ximage_cleanup (zbar_window_t *w)
{
    if(w->img.x)
        free(w->img.x);
    w->img.x = NULL;
    return(0);
}

static inline int ximage_init (zbar_window_t *w,
                               zbar_image_t *img)
{
    if(w->img.x) {
        free(w->img.x);
        w->img.x = NULL;
    }
    if(w->src_format != img->format &&
       w->format != img->format) {
        _zbar_best_format(img->format, &w->format, w->formats);
        if(!w->format) {
            err_capture_int(w, SEV_ERROR, ZBAR_ERR_UNSUPPORTED, __func__,
                            "no conversion from %x to supported formats",
                            img->format);
            return(-1);
        }
        w->src_format = img->format;
    }
    XImage *ximg = w->img.x = calloc(1, sizeof(XImage));
    ximg->width = img->width;
    ximg->height = img->height;
    ximg->format = ZPixmap;
    ximg->byte_order = LSBFirst;
    ximg->bitmap_unit = 8;
    ximg->bitmap_bit_order = MSBFirst;
    ximg->bitmap_pad = 8;

    const zbar_format_def_t *fmt = _zbar_format_lookup(w->format);
    if(fmt->group == ZBAR_FMT_RGB_PACKED) {
        ximg->depth = ximg->bits_per_pixel = fmt->p.rgb.bpp << 3;
        ximg->red_mask =
            (0xff >> RGB_SIZE(fmt->p.rgb.red))
            << RGB_OFFSET(fmt->p.rgb.red);
        ximg->green_mask =
            (0xff >> RGB_SIZE(fmt->p.rgb.green))
            << RGB_OFFSET(fmt->p.rgb.green);
        ximg->blue_mask =
            (0xff >> RGB_SIZE(fmt->p.rgb.blue))
            << RGB_OFFSET(fmt->p.rgb.blue);
    }
    else {
        ximg->depth = ximg->bits_per_pixel = 8;
        ximg->red_mask = ximg->green_mask = ximg->blue_mask = 0xff;
    }

    if(!XInitImage(ximg))
        return(err_capture_int(w, SEV_ERROR, ZBAR_ERR_XPROTO, __func__,
                               "unable to init XImage for format %x",
                               w->format));
    zprintf(3, "new XImage %.4s(%08" PRIx32 ") %dx%d"
            " from %.4s(%08" PRIx32 ") %dx%d\n",
            (char*)&w->format, w->format, ximg->width, ximg->height,
            (char*)&img->format, img->format, img->width, img->height);
    zprintf(4, "    masks: %08lx %08lx %08lx\n",
            ximg->red_mask, ximg->green_mask, ximg->blue_mask);
    return(0);
}

static int ximage_draw (zbar_window_t *w,
                        zbar_image_t *img)
{
    XImage *ximg = w->img.x;
    if(!ximg ||
       (w->src_format != img->format &&
        w->format != img->format) ||
       ximg->width != img->width ||
       ximg->height != img->height) {
        if(ximage_init(w, img))
            return(-1);
        ximg = w->img.x;
    }
    if(img->format != w->format) {
        /* save *converted* image for redraw */
        w->image = zbar_image_convert(img, w->format);
        zbar_image_destroy(img);
        img = w->image;
    }

    ximg->data = (void*)img->data;

    int screen = DefaultScreen(w->display);
    XSetForeground(w->display, w->gc, WhitePixel(w->display, screen));

    /* FIXME implement some basic scaling */
    unsigned height = img->height;
    unsigned src_y = 0, dst_y = 0;
    if(w->height < img->height) {
        height = w->height;
        src_y = (img->height - w->height) >> 1;
    }
    else if(w->height != img->height) {
        dst_y = (w->height - img->height) >> 1;
        /* fill border */
        XFillRectangle(w->display, w->xwin, w->gc,
                       0, 0, w->width, dst_y);
        XFillRectangle(w->display, w->xwin, w->gc,
                       0, dst_y + img->height, w->width, dst_y);
    }

    unsigned width = img->width;
    unsigned src_x = 0, dst_x = 0;
    if(w->width < img->width) {
        width = w->width;
        src_x = (img->width - w->width) >> 1;
    }
    else if(w->width != img->width) {
        dst_x = (w->width - img->width) >> 1;
        /* fill border */
        XFillRectangle(w->display, w->xwin, w->gc,
                       0, dst_y, dst_x, img->height);
        XFillRectangle(w->display, w->xwin, w->gc,
                       img->width + dst_x, dst_y, dst_x, img->height);
    }

    XPutImage(w->display, w->xwin, w->gc, ximg,
              src_x, src_y, dst_x, dst_y, width, height);
    ximg->data = NULL;
    return(0);
}

static uint32_t ximage_formats[4][5] = {
    {   /* 8bpp */
        /* FIXME fourcc('Y','8','0','0'), */
        fourcc('R','G','B','1'),
        fourcc('B','G','R','1'),
        0
    },
    {   /* 16bpp */
        fourcc('R','G','B','P'), fourcc('R','G','B','O'),
        fourcc('R','G','B','R'), fourcc('R','G','B','Q'),
        0
    },
    {   /* 24bpp */
        fourcc('R','G','B','3'),
        fourcc('B','G','R','3'),
        0
    },
    {   /* 32bpp */
        fourcc('R','G','B','4'),
        fourcc('B','G','R','4'),
        0
    },
};

static int ximage_probe_format (zbar_window_t *w,
                                uint32_t format)
{
    const zbar_format_def_t *fmt = _zbar_format_lookup(format);

    XVisualInfo visreq, *visuals = NULL;
    memset(&visreq, 0, sizeof(XVisualInfo));

    visreq.depth = fmt->p.rgb.bpp << 3;
    visreq.red_mask =
        (0xff >> RGB_SIZE(fmt->p.rgb.red)) << RGB_OFFSET(fmt->p.rgb.red);
    visreq.green_mask =
        (0xff >> RGB_SIZE(fmt->p.rgb.green)) << RGB_OFFSET(fmt->p.rgb.green);
    visreq.blue_mask =
        (0xff >> RGB_SIZE(fmt->p.rgb.blue)) << RGB_OFFSET(fmt->p.rgb.blue);

    int n;
    visuals = XGetVisualInfo(w->display,
                             VisualDepthMask | VisualRedMaskMask |
                             VisualGreenMaskMask | VisualBlueMaskMask,
                             &visreq, &n);

    zprintf(8, "bits=%d r=%08lx g=%08lx b=%08lx: n=%d visuals=%p\n",
            visreq.depth, visreq.red_mask, visreq.green_mask,
            visreq.blue_mask, n, visuals);
    if(!visuals)
        return(1);
    XFree(visuals);
    if(!n)
        return(-1);

    return(0);
}

int _zbar_window_probe_ximage (zbar_window_t *w)
{
    /* FIXME determine supported formats/depths */
    int n;
    XPixmapFormatValues *formats = XListPixmapFormats(w->display, &n);
    if(!formats)
        return(err_capture(w, SEV_ERROR, ZBAR_ERR_XPROTO, __func__,
                           "unable to query XImage formats"));

    int i;
    for(i = 0; i < n; i++) {
        if(formats[i].depth & 0x7 ||
           formats[i].depth > 0x20) {
            zprintf(2, "    [%d] depth=%d bpp=%d: not supported\n",
                    i, formats[i].depth, formats[i].bits_per_pixel);
            continue;
        }
        int fmtidx = formats[i].depth / 8 - 1;
        int j, n = 0;
        for(j = 0; ximage_formats[fmtidx][j]; j++)
            if(!ximage_probe_format(w, ximage_formats[fmtidx][j])) {
                zprintf(2, "    [%d] depth=%d bpp=%d: %.4s(%08" PRIx32 ")\n",
                        i, formats[i].depth, formats[i].bits_per_pixel,
                        (char*)&ximage_formats[fmtidx][j],
                        ximage_formats[fmtidx][j]);
                _zbar_window_add_format(w, ximage_formats[fmtidx][j]);
                n++;
            }
        if(!n)
            zprintf(2, "    [%d] depth=%d bpp=%d: no visuals\n",
                    i, formats[i].depth, formats[i].bits_per_pixel);
    }
    XFree(formats);

    if(!w->formats || !w->formats[0])
        return(err_capture(w, SEV_ERROR, ZBAR_ERR_UNSUPPORTED, __func__,
                           "no usable XImage formats found"));

    w->draw_image = ximage_draw;
    w->cleanup = ximage_cleanup;
    return(0);
}
