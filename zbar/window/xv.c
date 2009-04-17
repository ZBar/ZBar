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
#include <string.h>     /* strcmp */

static int xv_cleanup (zbar_window_t *w)
{
    if(w->img.xv) {
        XFree(w->img.xv);
        w->img.xv = NULL;
    }
    int i;
    for(i = 0; i < w->num_xv_adaptors; i++)
        if(w->xv_adaptors[i]) {
            XvUngrabPort(w->display, w->xv_adaptors[i], CurrentTime);
            w->xv_adaptors[i] = 0;
        }
    free(w->xv_ports);
    free(w->xv_adaptors);
    w->xv_ports = NULL;
    w->num_xv_adaptors = 0;
    w->xv_adaptors = NULL;
    return(0);
}

static inline int xv_init (zbar_window_t *w,
                           zbar_image_t *img)
{
    if(w->img.xv) {
        XFree(w->img.xv);
        w->img.xv = NULL;
    }
    if(w->src_format != img->format &&
       w->format != img->format) {
        _zbar_best_format(img->format, &w->format, w->formats);
        if(!w->format) {
            err_capture_int(w, SEV_ERROR, ZBAR_ERR_UNSUPPORTED, __func__,
                            "no conversion from %x to supported Xv formats",
                            img->format);
            return(-1);
        }
        w->src_format = img->format;
        /* lookup port for format */
        w->img_port = 0;
        int i;
        for(i = 0; w->formats[i]; i++)
            if(w->formats[i] == w->format) {
                w->img_port = w->xv_ports[i];
                break;
            }
        assert(w->img_port > 0);
    }
    w->src_width = img->width;
    w->src_height = img->height;
    XvImage *xvimg = XvCreateImage(w->display, w->img_port, w->format,
                                   NULL, img->width, img->height);
    zprintf(3, "new XvImage %.4s(%08" PRIx32 ") %dx%d(%d)"
            " from %.4s(%08" PRIx32 ") %dx%d\n",
            (char*)&w->format, w->format,
            xvimg->width, xvimg->height, xvimg->pitches[0],
            (char*)&img->format, img->format, img->width, img->height);

    /* FIXME not sure this simple check is always correct
     * should lookup format to decode/sanitize target width from pitch & bpp
     */
    w->dst_width = ((xvimg->num_planes <= 1)
                    ? xvimg->width
                    : xvimg->pitches[0]);

    /* FIXME datalen check */
    if(w->dst_width < img->width || xvimg->height < img->height) {
        XFree(xvimg);
        /* FIXME fallback to XImage... */
        return(err_capture(w, SEV_ERROR, ZBAR_ERR_UNSUPPORTED, __func__,
                           "output image size mismatch (XvCreateImage)"));
    }
    w->img.xv = xvimg;

    return(0);
}

static int xv_draw (zbar_window_t *w,
                    zbar_image_t *img)
{
    XvImage *xvimg = w->img.xv;
    /* FIXME preserve aspect ratio (config?) */
    if(!xvimg ||
       (img->format != w->src_format &&
        img->format != w->format) ||
       (img->width != w->src_width  &&
        img->width != w->dst_width) ||
       (img->height != w->src_height  &&
        img->height != xvimg->height)) {
        if(xv_init(w, img))
            return(-1);
        xvimg = w->img.xv;
    }
    if(img->format != w->format ||
       img->width != w->dst_width ||
       img->height != xvimg->height) {
        w->src_width = img->width;
        w->src_height = img->height;
        /* save *converted* image for redraw */
        w->image = zbar_image_convert_resize(img, w->format,
                                              w->dst_width,
                                              xvimg->height);
        zbar_image_destroy(img);
        img = w->image;
    }

    xvimg->data = (void*)img->data;
    zprintf(24, "XvPutImage(%dx%d -> %dx%d)\n",
            w->src_width, w->src_height, w->width, w->height);
    XvPutImage(w->display, w->img_port, w->xwin, w->gc, xvimg,
               0, 0, w->src_width, w->src_height,
               0, 0, w->width, w->height);
    xvimg->data = NULL;  /* FIXME hold shm image until completion */
    return(0);
}

static inline int xv_add_format (zbar_window_t *w,
                                 uint32_t fmt,
                                 XvPortID port)
{
    int i = _zbar_window_add_format(w, fmt);

    if(!w->formats[i + 1])
        w->xv_ports = realloc(w->xv_ports, (i + 1) * sizeof(uint32_t));

    /* FIXME could prioritize by something (rate? size?) */
    w->xv_ports[i] = port;
    return(i);
}

static int xv_probe_port (zbar_window_t *w,
                          XvPortID port)
{
    unsigned n;
    XvEncodingInfo *encodings = NULL;
    if(XvQueryEncodings(w->display, port, &n, &encodings))
        return(err_capture(w, SEV_ERROR, ZBAR_ERR_XPROTO, __func__,
                           "querying XVideo encodings"));

    zprintf(1, "probing port %u with %d encodings:\n", (unsigned)port, n);
    unsigned width = 0, height = 0;
    int i;
    for(i = 0; i < n; i++) {
        XvEncodingInfo *enc = &encodings[i];
        zprintf(2, "    [%d] %lu x %lu rate=%d/%d : %s\n",
                i, enc->width, enc->height,
                enc->rate.numerator, enc->rate.denominator, enc->name);
        if(!strcmp(enc->name, "XV_IMAGE")) {
            if(width < enc->width)
                width = enc->width;
            if(height < enc->height)
                height = enc->height;
        }
    }
    XvFreeEncodingInfo(encodings);
    encodings = NULL;

    if(!width || !height)
        return(err_capture(w, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "no XV_IMAGE encodings found"));
    zprintf(1, "max XV_IMAGE size %dx%d\n", width, height);
    if(w->max_width > width)
        w->max_width = width;
    if(w->max_height > height)
        w->max_height = height;

    XvImageFormatValues *formats =
        XvListImageFormats(w->display, port, (int*)&n);
    if(!formats)
        return(err_capture(w, SEV_ERROR, ZBAR_ERR_XPROTO, __func__,
                           "querying XVideo image formats"));

    zprintf(1, "%d image formats\n", n);
    for(i = 0; i < n; i++) {
        XvImageFormatValues *fmt = &formats[i];
        zprintf(2, "    [%d] %.4s(%08x) %s %s %s planes=%d bpp=%d : %.16s\n",
                i, (char*)&fmt->id, fmt->id,
                (fmt->type == XvRGB) ? "RGB" : "YUV",
                (fmt->byte_order == LSBFirst) ? "LSBFirst" : "MSBFirst",
                (fmt->format == XvPacked) ? "packed" : "planar",
                fmt->num_planes, fmt->bits_per_pixel, fmt->guid);
        xv_add_format(w, fmt->id, port);
    }
    XFree(formats);
    return(0);
}

int _zbar_window_probe_xv (zbar_window_t *w)
{
    unsigned xv_major, xv_minor, xv_req, xv_ev, xv_err;
    if(XvQueryExtension(w->display, &xv_major, &xv_minor,
                        &xv_req, &xv_ev, &xv_err)) {
        zprintf(1, "XVideo extension not present\n");
        return(-1);
    }
    zprintf(1, "XVideo extension version %u.%u\n",
            xv_major, xv_minor);

    unsigned n;
    XvAdaptorInfo *adaptors = NULL;
    if(XvQueryAdaptors(w->display, w->xwin, &n, &adaptors))
        return(err_capture(w, SEV_ERROR, ZBAR_ERR_XPROTO, __func__,
                           "unable to query XVideo adaptors"));

    w->num_xv_adaptors = 0;
    w->xv_adaptors = calloc(n, sizeof(int));
    int i;
    for(i = 0; i < n; i++) {
        XvAdaptorInfo *adapt = &adaptors[i];
        zprintf(2, "    adaptor[%d] %lu ports %u-%u type=0x%x fmts=%lu : %s\n",
                i, adapt->num_ports, (unsigned)adapt->base_id,
                (unsigned)(adapt->base_id + adapt->num_ports - 1),
                adapt->type, adapt->num_formats, adapt->name);
        if(!(adapt->type & XvImageMask))
            continue;

        int j;
        for(j = 0; j < adapt->num_ports; j++)
            if(!XvGrabPort(w->display, adapt->base_id + j, CurrentTime)) {
                zprintf(3, "        grabbed port %u\n",
                        (unsigned)(adapt->base_id + j));
                w->xv_adaptors[w->num_xv_adaptors++] = adapt->base_id + j;
                break;
            }

        if(j == adapt->num_ports)
            zprintf(3, "        no available XVideo image port\n");
    }
    XvFreeAdaptorInfo(adaptors);
    adaptors = NULL;

    if(!w->num_xv_adaptors) {
        zprintf(1, "WARNING: no XVideo adaptor supporting XvImages found\n");
        free(w->xv_adaptors);
        w->xv_adaptors = NULL;
        return(-1);
    }
    if(w->num_xv_adaptors < n)
        w->xv_adaptors = realloc(w->xv_adaptors,
                                 w->num_xv_adaptors * sizeof(int));

    w->max_width = w->max_height = 65536;
    w->formats = realloc(w->formats, sizeof(uint32_t));
    w->formats[0] = 0;
    for(i = 0; i < w->num_xv_adaptors; i++)
        if(xv_probe_port(w, w->xv_adaptors[i])) {
            XvUngrabPort(w->display, w->xv_adaptors[i], CurrentTime);
            w->xv_adaptors[i] = 0;
        }
    if(!w->formats[0] || w->max_width == 65536 || w->max_height == 65536) {
        xv_cleanup(w);
        return(-1);
    }

    /* clean out any unused adaptors */
    for(i = 0; i < w->num_xv_adaptors; i++) {
        int j;
        for(j = 0; w->formats[j]; j++)
            if(w->xv_ports[j] == w->xv_adaptors[i])
                break;
        if(!w->formats[j]) {
            XvUngrabPort(w->display, w->xv_adaptors[i], CurrentTime);
            w->xv_adaptors[i] = 0;
        }
    }

    w->draw_image = xv_draw;
    w->cleanup = xv_cleanup;
    return(0);
}
