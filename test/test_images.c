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

#include <config.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <zbar.h>
#include "test_images.h"

typedef enum format_type_e {
    GRAY, YUVP, YVUP, YUYV, YVYU, UYVY,
    RGB888, BGR888,
    RGB565B = 0x0565, RGB565L = 0x1565,
    RGB555B = 0x0555, RGB555L = 0x1555, 
} format_type_t;

typedef struct format_def_s {
    uint32_t format;
    format_type_t type;
    uint8_t bpp;
    uint8_t xdiv, ydiv;
} format_def_t;

typedef union packed_u {
    uint32_t u32[3];
    uint16_t u16[6];
    uint8_t u8[12];
} packed_t;

/* bar colors */
static const uint8_t Cr[] = {
    0x22, 0x92, 0x80, 0xf0, 0x10, 0x80, 0x6e, 0xde
};
static const uint8_t Cb[] = {
    0x36, 0x10, 0x80, 0x5a, 0xa6, 0x80, 0xf0, 0xca
};

static const format_def_t formats[] = {
    { fourcc('G','R','E','Y'), GRAY,  8, 0,0 },
    { fourcc('Y','8','0','0'), GRAY,  8, 0,0 },
    { fourcc('Y','8',' ',' '), GRAY,  8, 0,0 },
    { fourcc('Y','8', 0 , 0 ), GRAY,  8, 0,0 },

    { fourcc('Y','U','V','9'), YUVP,  9, 4,4 },
    { fourcc('Y','V','U','9'), YVUP,  9, 4,4 },

    { fourcc('I','4','2','0'), YUVP, 12, 2,2 },
    { fourcc('Y','U','1','2'), YUVP, 12, 2,2 },
    { fourcc('Y','V','1','2'), YVUP, 12, 2,2 },
    { fourcc('4','1','1','P'), YUVP, 12, 4,1 },

    { fourcc('N','V','1','2'), YUVP, 12, 2,2 },
    { fourcc('N','V','2','1'), YVUP, 12, 2,2 },

    { fourcc('4','2','2','P'), YUVP, 16, 2,1 },

    { fourcc('Y','U','Y','V'), YUYV, 16, 2,1 },
    { fourcc('Y','U','Y','2'), YUYV, 16, 2,1 },
    { fourcc('Y','V','Y','U'), YVYU, 16, 2,1 },
    { fourcc('U','Y','V','Y'), UYVY, 16, 2,1 },

    { fourcc('R','G','B','3'), RGB888,  24, },
    { fourcc('B','G','R','3'), BGR888,  24, },
    { fourcc( 3 , 0 , 0 , 0 ), RGB888,  32, },
    { fourcc('R','G','B','4'), RGB888,  32, },
    { fourcc('B','G','R','4'), BGR888,  32, },
    { fourcc('R','G','B','P'), RGB565L, 16, },
    { fourcc('R','G','B','O'), RGB555L, 16, },
    { fourcc('R','G','B','R'), RGB565B, 16, },
    { fourcc('R','G','B','Q'), RGB555B, 16, },
    { 0 }
};

static const char *encoded_widths =
    "9 111 212241113121211311141132 11111 311213121312121332111132 111 9";
const char *test_image_ean13_data = "6268964977804";

static int allocated_images = 0;

int
test_image_check_cleanup ()
{
    if(allocated_images)
        fprintf(stderr, "ERROR: %d image data buffers still allocated\n",
                allocated_images);
    /*else
        fprintf(stderr, "all image data buffers freed\n");*/
    return(allocated_images);
}

static void
test_cleanup_handler (zbar_image_t *img)
{
    void *data = (void*)zbar_image_get_data(img);
    /*fprintf(stderr, "cleanup image data @%p\n", data);*/
    free(data);
    allocated_images--;
}

static inline const format_def_t*
lookup_format (zbar_image_t *img)
{
    uint32_t ifmt = zbar_image_get_format(img);
    const format_def_t *fmt;
    for(fmt = formats; fmt->format; fmt++)
        if(fmt->format == ifmt)
            break;
    if(!fmt->format) {
        fprintf(stderr, "ERROR: no %.4s (%08"PRIx32") format\n",
                (char*)&ifmt, ifmt);
        return(NULL);
    }
    return(fmt);
}

static inline const format_def_t*
alloc_data (zbar_image_t *img)
{
    allocated_images++;
    const format_def_t *fmt = lookup_format(img);
    if(!fmt)
        return(NULL);

    unsigned w = zbar_image_get_width(img);
    unsigned h = zbar_image_get_height(img);
    unsigned long planelen = w * h;
    unsigned long datalen = planelen * fmt->bpp / 8;
    uint8_t *data = malloc(datalen);

    zbar_image_set_data(img, data, datalen, test_cleanup_handler);

    /*fprintf(stderr, "create %.4s(%08"PRIx32") image data %lx bytes @%p\n",
      (char*)&fmt->format, fmt->format, datalen, data);*/
    return(fmt);
}

/* write intensity plane */
static inline uint8_t*
fill_bars_y (uint8_t *p,
             unsigned w,
             unsigned h)
{
    unsigned x, y, i;
    unsigned y0 = (h + 31) / 30;
    for(y = 0; y < y0; y++)
        for(x = 0; x < w; x++)
            *(p++) = 0xff;

    for(; y < h - y0; y++)
        for(x = 0, i = 0; x < w; i++) {
            assert(i < 8);
            unsigned x0 = (((i + 1) * w) + 7) >> 3;
            assert(x0 <= w);
            unsigned v = ((((i & 1) ? y : h - y) * 256) + h - 1) / h;
            for(; x < x0; x++)
                *(p++) = v;
        }

    for(; y < h; y++)
        for(x = 0; x < w; x++)
            *(p++) = 0xff;

    return(p);
}

/* write Cb (U) or Cr (V) plane */
static inline uint8_t*
fill_bars_uv (uint8_t *p,
              unsigned w,
              unsigned h,
              const uint8_t *C)
{
    unsigned x, y, i;
    unsigned y0 = (h + 31) / 30;

    for(y = 0; y < y0; y++)
        for(x = 0; x < w; x++)
            *(p++) = 0x80;

    for(; y < h - y0; y++)
        for(x = 0, i = 0; x < w; i++) {
            assert(i < 8);
            unsigned x0 = (((i + 1) * w) + 7) >> 3;
            assert(x0 <= w);
            for(; x < x0; x++)
                *(p++) = C[i];
        }

    for(; y < h; y++)
        for(x = 0; x < w; x++)
            *(p++) = 0x80;

    return(p);
}

/* write packed CbCr plane */
static inline uint8_t*
fill_bars_nv (uint8_t *p,
              unsigned w,
              unsigned h,
              format_type_t order)
{
    unsigned x, y, i;
    unsigned y0 = (h + 31) / 30;

    for(y = 0; y < y0; y++)
        for(x = 0; x < w; x++) {
            *(p++) = 0x80;  *(p++) = 0x80;
        }

    for(; y < h - y0; y++)
        for(x = 0, i = 0; x < w; i++) {
            assert(i < 8);
            unsigned x0 = (((i + 1) * w) + 7) >> 3;
            assert(x0 <= w);
            uint8_t u = (order == YUVP) ? Cb[i] : Cr[i];
            uint8_t v = (order == YUVP) ? Cr[i] : Cb[i];
            for(; x < x0; x++) {
                *(p++) = u;  *(p++) = v;
            }
        }

    for(; y < h; y++)
        for(x = 0; x < w; x++) {
            *(p++) = 0x80;  *(p++) = 0x80;
        }

    return(p);
}

/* write packed YCbCr plane */
static inline uint8_t*
fill_bars_yuv (uint8_t *p,
               unsigned w,
               unsigned h,
               format_type_t order)
{
    unsigned x, y, i;
    unsigned y0 = (h + 31) / 30;
    packed_t yuv;
    uint32_t *q = (uint32_t*)p;
    w /= 2;

    yuv.u8[0] = yuv.u8[2] = (order == UYVY) ? 0x80 : 0xff;
    yuv.u8[1] = yuv.u8[3] = (order == UYVY) ? 0xff : 0x80;
    for(y = 0; y < y0; y++)
        for(x = 0; x < w; x++)
            *(q++) = yuv.u32[0];

    for(; y < h - y0; y++)
        for(x = 0, i = 0; x < w; i++) {
            assert(i < 8);
            unsigned x0 = (((i + 1) * w) + 7) >> 3;
            assert(x0 <= w);
            unsigned v = ((((i & 1) ? y : h - y) * 256) + h - 1) / h;
            if(order == UYVY) {
                yuv.u8[0] = Cb[i];
                yuv.u8[2] = Cr[i];
                yuv.u8[1] = yuv.u8[3] = v;
            } else {
                yuv.u8[0] = yuv.u8[2] = v;
                yuv.u8[1] = (order == YUYV) ? Cb[i] : Cr[i];
                yuv.u8[3] = (order == YVYU) ? Cr[i] : Cb[i];
            }
            for(; x < x0; x++)
                *(q++) = yuv.u32[0];
        }

    yuv.u8[0] = yuv.u8[2] = (order == UYVY) ? 0x80 : 0xff;
    yuv.u8[1] = yuv.u8[3] = (order == UYVY) ? 0xff : 0x80;
    for(; y < h; y++)
        for(x = 0; x < w; x++)
            *(q++) = yuv.u32[0];

    return((uint8_t*)q);
}

static inline uint8_t*
fill_bars_rgb (uint8_t *p,
               unsigned w,
               unsigned h,
               format_type_t order,
               int bpp)
{
    unsigned x, y, i;
    unsigned y0 = (h + 31) / 30;
    packed_t rgb;

    unsigned headlen = y0 * w * bpp / 8;
    memset(p, 0xff, headlen);
    uint32_t *q = (uint32_t*)(p + headlen);

    for(y = y0; y < h - y0; y++)
        for(x = 0, i = 0; x < w; i++) {
            assert(i < 8);
            /* FIXME clean this up... */
            unsigned x0 = (((i + 1) * w) + 7) >> 3;
            assert(x0 <= w);
            unsigned yi = (i & 1) ? y : h - y;
            unsigned v1, v0;
            if(yi < h / 2 - 1) {
                v1 = ((yi * 0x180) + h - 1) / h + 0x40;
                v0 = 0x00;
            } else {
                v1 = 0xff;
                v0 = (((yi - (h / 2)) * 0x180) + h - 1) / h + 0x40;
            }

            uint8_t r = (i & 4) ? v1 : v0;
            uint8_t g = (i & 2) ? v1 : v0;
            uint8_t b = (i & 1) ? v1 : v0;
            if(bpp == 32) {
                if(order == RGB888) {
                    rgb.u8[0] = 0xff;
                    rgb.u8[1] = r;
                    rgb.u8[2] = g;
                    rgb.u8[3] = b;
                } else {
                    rgb.u8[0] = b;
                    rgb.u8[1] = g;
                    rgb.u8[2] = r;
                    rgb.u8[3] = 0xff;
                }
                for(; x < x0; x++)
                    *(q++) = rgb.u32[0];
            }
            else if(bpp == 24) {
                rgb.u8[0] = rgb.u8[3] = rgb.u8[6] = rgb.u8[9] =
                    (order == RGB888) ? r : b;
                rgb.u8[1] = rgb.u8[4] = rgb.u8[7] = rgb.u8[10] = g;
                rgb.u8[2] = rgb.u8[5] = rgb.u8[8] = rgb.u8[11] =
                    (order == RGB888) ? b : r;
                for(; x < x0; x += 4) {
                    *(q++) = rgb.u32[0];
                    *(q++) = rgb.u32[1];
                    *(q++) = rgb.u32[2];
                }
            }
            else {
                assert(bpp == 16);
                r = ((r + 7) / 8) & 0x1f;
                b = ((b + 7) / 8) & 0x1f;
                if((order & 0x0fff) == 0x0555) {
                    g = ((g + 7) / 8) & 0x1f;
                    rgb.u16[0] = b | (g << 5) | (r << 10);
                } else {
                    g = ((g + 3) / 4) & 0x3f;
                    rgb.u16[0] = b | (g << 5) | (r << 11);
                }
                if(order & 0x1000)
                    rgb.u16[0] = (rgb.u16[0] >> 8) | (rgb.u16[0] << 8);
                rgb.u16[1] = rgb.u16[0];
                for(; x < x0; x += 2)
                    *(q++) = rgb.u32[0];
            }
        }

    memset(q, 0xff, headlen);
    return(((uint8_t*)q) + headlen);
}

int
test_image_bars (zbar_image_t *img)
{
    const format_def_t *fmt = alloc_data(img);
    if(!fmt)
        return(-1);

    unsigned w = zbar_image_get_width(img);
    unsigned h = zbar_image_get_height(img);
    uint8_t *data = (void*)zbar_image_get_data(img);
    assert(data);
    uint8_t *p = data;
    switch(fmt->type) {
    case GRAY:
    case YUVP: /* planar YUV */
    case YVUP:
        p = fill_bars_y(p, w, h);
        if(fmt->type != GRAY) {
            w = (w + fmt->xdiv - 1) / fmt->xdiv;
            h = (h + fmt->ydiv - 1) / fmt->ydiv;
        }
        else
            break;

        if(fmt->format == fourcc('N','V','1','2') ||
           fmt->format == fourcc('N','V','2','1'))
            p = fill_bars_nv(p, w, h, fmt->type);
        else if(fmt->type == YUVP ||
                fmt->type == YVUP) {
            p = fill_bars_uv(p, w, h, (fmt->type == YUVP) ? Cb : Cr);
            p = fill_bars_uv(p, w, h, (fmt->type == YUVP) ? Cr : Cb);
        }
        break;

    case YUYV: /* packed YUV */
    case YVYU:
    case UYVY:
        p = fill_bars_yuv(p, w, h, fmt->type);
        break;

    default: /* RGB */
        p = fill_bars_rgb(p, w, h, fmt->type, fmt->bpp);
        break;
    }

    assert(p == data + zbar_image_get_data_length(img));
    return(0);
}

int
test_image_ean13 (zbar_image_t *img)
{
    unsigned w = 114, h = 85;
    zbar_image_set_size(img, w, h);

    const format_def_t *fmt = alloc_data(img);
    if(!fmt)
        return(-1);

    uint8_t *data = (void*)zbar_image_get_data(img);
    unsigned int datalen = zbar_image_get_data_length(img);
    assert(data && datalen);

    uint8_t *p = data;
    /* FIXME randomize? */
    memset(data, 0x80, datalen);

    int nrep = 1, nskip = 0;
    switch(fmt->type)
    {
    case YUVP: /* planar YUV */
    case YVUP:
    case GRAY:
        break;

    case UYVY: /* packed YUV */
        p++;
    case YUYV:
    case YVYU:
        nskip = 1;
        break;

    default: /* RGB */
        nrep = fmt->bpp / 8;
    }

    int y = 0, x, i;
    for(; y < 10 && y < h; y++)
        for(x = 0; x < w; x++) {
            for(i = 0; i < nrep; i++)
                *p++ = 0xff;
            p += nskip;
        }

    for(; y < h - 10; y++) {
        uint8_t color = 0xff;
        const char *c;
        for(x = 0, c = encoded_widths; *c; c++) {
            int dx;
            if(*c == ' ')
                continue;
            for(dx = *c - '0'; dx > 0; dx--) {
                for(i = 0; i < nrep; i++)
                    *p++ = color;
                p += nskip;
                x++;
            }
            color = ~color;
        }
        assert(!color);
        for(; x < w; x++) {
            for(i = 0; i < nrep; i++)
                *p++ = 0xff;
            p += nskip;
        }
        assert(x == w);
    }

    for(; y < h; y++)
        for(x = 0; x < w; x++) {
            for(i = 0; i < nrep; i++)
                *p++ = 0xff;
            p += nskip;
        }

    if(fmt->type == UYVY)
        p--;
    assert(p == data + datalen);
    return(0);
}
