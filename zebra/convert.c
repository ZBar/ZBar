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
#include "video.h"
#include "window.h"

/* pack bit size and location offset of a component into one byte
 */
#define RGB_BITS(off, size) ((((8 - (size)) & 0x7) << 5) | ((off) & 0x1f))

extern int _zebra_video_init(zebra_video_t*, uint32_t);

typedef void (conversion_handler_t)(zebra_image_t*,
                                    const zebra_format_def_t*,
                                    const zebra_image_t*,
                                    const zebra_format_def_t*);

typedef struct conversion_def_s {
    int cost;                           /* conversion "badness" */
    conversion_handler_t *func;         /* function that accomplishes it */
} conversion_def_t;


/* NULL terminated list of known formats, in order of preference
 * (NB Cr=V Cb=U)
 */
static const uint32_t format_prefs[] = {

    /* planar YUV formats */
    fourcc('4','2','2','P'), /* FIXME also YV16? */
    fourcc('I','4','2','0'),
    fourcc('Y','U','1','2'), /* FIXME also IYUV? */
    fourcc('Y','V','1','2'),
    fourcc('4','1','1','P'),

    /* planar Y + packed UV plane */
    fourcc('N','V','1','2'),
    fourcc('N','V','2','1'),

    /* packed YUV formats */
    fourcc('Y','U','Y','V'),
    fourcc('U','Y','V','Y'),
    fourcc('Y','U','Y','2'), /* FIXME add YVYU */
    fourcc('Y','U','V','4'), /* FIXME where is this from? */

    /* packed rgb formats */
    fourcc('R','G','B','3'),
    fourcc( 3 , 0 , 0 , 0 ),
    fourcc('B','G','R','3'),
    fourcc('R','G','B','4'),
    fourcc('B','G','R','4'),

    fourcc('R','G','B','P'),
    fourcc('R','G','B','O'),
    fourcc('R','G','B','R'),
    fourcc('R','G','B','Q'),

    fourcc('Y','U','V','9'),
    fourcc('Y','V','U','9'),

    /* basic grayscale format */
    fourcc('G','R','E','Y'),
    fourcc('Y','8','0','0'),
    fourcc('Y','8',' ',' '),
    fourcc('Y','8', 0 , 0 ),

    /* low quality RGB formats */
    fourcc('R','G','B','1'),
    fourcc('R','4','4','4'),
    fourcc('B','A','8','1'),

    /* unsupported packed YUV formats */
    fourcc('Y','4','1','P'),
    fourcc('Y','4','4','4'),
    fourcc('Y','U','V','O'),
    fourcc('H','M','1','2'),

    /* unsupported packed RGB format */
    fourcc('H','I','2','4'),

    /* unsupported compressed formats */
    fourcc('J','P','E','G'),
    fourcc('M','J','P','G'),
    fourcc('M','P','E','G'),

    /* terminator */
    0
};

static const int num_format_prefs =
    sizeof(format_prefs) / sizeof(zebra_format_def_t);

/* format definitions */
static const zebra_format_def_t format_defs[] = {

    { fourcc('B','G','R','4'), ZEBRA_FMT_RGB_PACKED,
        { { 4, RGB_BITS(16, 8), RGB_BITS(8, 8), RGB_BITS(0, 8) } } },
    { fourcc('Y','U','1','2'), ZEBRA_FMT_YUV_PLANAR, { { 1, 1, 0 /*UV*/ } } },
    { fourcc('R','G','B','R'), ZEBRA_FMT_RGB_PACKED,
        { { 2, RGB_BITS(3, 5), RGB_BITS(13, 6), RGB_BITS(8, 5) } } },
    { fourcc('Y','8','0','0'), ZEBRA_FMT_GRAY, },
    { fourcc('R','G','B','3'), ZEBRA_FMT_RGB_PACKED,
        { { 3, RGB_BITS(0, 8), RGB_BITS(8, 8), RGB_BITS(16, 8) } } },
    { fourcc('4','1','1','P'), ZEBRA_FMT_YUV_PLANAR, { { 2, 0, 0 /*UV*/ } } },
    { fourcc('G','R','E','Y'), ZEBRA_FMT_GRAY, },
    { fourcc('Y','8', 0 , 0 ), ZEBRA_FMT_GRAY, },
    { fourcc('N','V','2','1'), ZEBRA_FMT_YUV_NV,     { { 1, 1, 1 /*VU*/ } } },
    { fourcc('Y','V','1','2'), ZEBRA_FMT_YUV_PLANAR, { { 1, 1, 1 /*VU*/ } } },
    { fourcc('R','4','4','4'), ZEBRA_FMT_RGB_PACKED,
        { { 2, RGB_BITS(8, 4), RGB_BITS(4, 4), RGB_BITS(0, 4) } } },
    { fourcc('Y','U','V','9'), ZEBRA_FMT_YUV_PLANAR, { { 2, 2, 0 /*UV*/ } } },
    { fourcc('R','G','B','P'), ZEBRA_FMT_RGB_PACKED,
        { { 2, RGB_BITS(11, 5), RGB_BITS(5, 6), RGB_BITS(0, 5) } } },
    { fourcc('Y','U','Y','V'), ZEBRA_FMT_YUV_PACKED,
        { { 1, 0, 0, /*YUYV*/ } } },
    { fourcc('U','Y','V','Y'), ZEBRA_FMT_YUV_PACKED,
        { { 1, 0, 2, /*UYVY*/ } } },
    { fourcc( 3 , 0 , 0 , 0 ), ZEBRA_FMT_RGB_PACKED,
        { { 4, RGB_BITS(8, 8), RGB_BITS(16, 8), RGB_BITS(24, 8) } } },
    { fourcc('Y','8',' ',' '), ZEBRA_FMT_GRAY, },
    { fourcc('I','4','2','0'), ZEBRA_FMT_YUV_PLANAR, { { 1, 1, 0 /*UV*/ } } },
    { fourcc('R','G','B','1'), ZEBRA_FMT_RGB_PACKED,
        { { 1, RGB_BITS(5, 3), RGB_BITS(2, 3), RGB_BITS(0, 2) } } },
    { fourcc('N','V','1','2'), ZEBRA_FMT_YUV_NV,     { { 1, 1, 0 /*UV*/ } } },
    { fourcc('Y','U','Y','2'), ZEBRA_FMT_YUV_PACKED,
        { { 1, 0, 0, /*YUYV*/ } } },
    { fourcc('B','G','R','3'), ZEBRA_FMT_RGB_PACKED,
        { { 3, RGB_BITS(16, 8), RGB_BITS(8, 8), RGB_BITS(0, 8) } } },
    { fourcc('R','G','B','4'), ZEBRA_FMT_RGB_PACKED,
        { { 4, RGB_BITS(8, 8), RGB_BITS(16, 8), RGB_BITS(24, 8) } } },
    { fourcc('Y','V','U','9'), ZEBRA_FMT_YUV_PLANAR, { { 2, 2, 1 /*VU*/ } } },
    { fourcc('R','G','B','O'), ZEBRA_FMT_RGB_PACKED,
        { { 2, RGB_BITS(10, 5), RGB_BITS(5, 5), RGB_BITS(0, 5) } } },
    { fourcc('4','2','2','P'), ZEBRA_FMT_YUV_PLANAR, { { 1, 0, 0 /*UV*/ } } },
    { fourcc('R','G','B','Q'), ZEBRA_FMT_RGB_PACKED,
        { { 2, RGB_BITS(2, 5), RGB_BITS(13, 5), RGB_BITS(8, 5) } } },
    { fourcc('Y','V','Y','U'), ZEBRA_FMT_YUV_PACKED,
        { { 1, 0, 1, /*YVYU*/ } } },
};

static const int num_format_defs =
    sizeof(format_defs) / sizeof(zebra_format_def_t);

#ifdef DEBUG_CONVERT
static int intsort (const void *a,
                    const void *b)
{
    return(*(uint32_t*)a - *(uint32_t*)b);
}
#endif

/* verify that format list is in required sort order */
static inline int verify_format_sort ()
{
    int i;
    for(i = 0; i < num_format_defs; i++) {
        int j = i * 2 + 1;
        if((j < num_format_defs &&
            format_defs[i].format < format_defs[j].format) ||
           (j + 1 < num_format_defs &&
            format_defs[j + 1].format < format_defs[i].format))
            break;
    }
    if(i == num_format_defs)
        return(0);

    /* spew correct order for fix */
    fprintf(stderr, "ERROR: image format list is not sorted!?\n");

#ifdef DEBUG_CONVERT
    assert(num_format_defs);
    uint32_t sorted[num_format_defs];
    uint32_t ordered[num_format_defs];
    for(i = 0; i < num_format_defs; i++)
        sorted[i] = format_defs[i].format;
    qsort(sorted, num_format_defs, sizeof(uint32_t), intsort);
    for(i = 0; i < num_format_defs; i = i << 1 | 1);
    i = (i - 1) / 2;
    ordered[i] = sorted[0];
    int j, k;
    for(j = 1; j < num_format_defs; j++) {
        k = i * 2 + 2;
        if(k < num_format_defs) {
            i = k;
            for(k = k * 2 + 1; k < num_format_defs; k = k * 2 + 1)
                i = k;
        }
        else {
            for(k = (i - 1) / 2; i != k * 2 + 1; k = (i - 1) / 2) {
                assert(i);
                i = k;
            }
            i = k;
        }
        ordered[i] = sorted[j];
    }
    fprintf(stderr, "correct sort order is:");
    for(i = 0; i < num_format_defs; i++)
        fprintf(stderr, " %4.4s", (char*)&ordered[i]);
    fprintf(stderr, "\n");
#endif
    return(-1);
}

static inline void uv_roundup (zebra_image_t *img,
                               const zebra_format_def_t *fmt)
{
    unsigned xmask = (1 << fmt->p.yuv.xsub2) - 1;
    if(img->width & xmask)
        img->width = (img->width + xmask) & ~xmask;
    unsigned ymask = (1 << fmt->p.yuv.ysub2) - 1;
    if(img->height & ymask)
        img->height = (img->height + ymask) & ~ymask;
}

static inline size_t uvp_size (const zebra_image_t *img,
                               const zebra_format_def_t *fmt)
{
    return((img->width >> fmt->p.yuv.xsub2) *
           (img->height >> fmt->p.yuv.ysub2));
}

static inline uint32_t convert_read_rgb (const uint8_t *srcp,
                                         int bpp)
{
    uint32_t p;
    if(bpp == 3) {
        p = *srcp;
        p |= *(srcp + 1) << 8;
        p |= *(srcp + 2) << 16;
    }
    else if(bpp == 4)
        p = *((uint32_t*)(srcp));
    else if(bpp == 2)
        p = *((uint16_t*)(srcp));
    else
        p = *srcp;
    return(p);
}

static inline void convert_write_rgb (uint8_t *dstp,
                                      uint32_t p,
                                      int bpp)
{
    if(bpp == 3) {
        *dstp = p & 0xff;
        *(dstp + 1) = (p >> 8) & 0xff;
        *(dstp + 2) = (p >> 16) & 0xff;
    }
    else if(bpp == 4)
        *((uint32_t*)dstp) = p;
    else if(bpp == 2)
        *((uint16_t*)dstp) = p;
    else
        *dstp = p;
}

/* cleanup linked image by unrefing */
static void cleanup_ref (zebra_image_t *img)
{
    if(img->next)
        _zebra_image_refcnt(img->next, -1);
}

/* make new image w/reference to the same image data */
static void convert_copy (zebra_image_t *dst,
                          const zebra_format_def_t *dstfmt,
                          const zebra_image_t *src,
                          const zebra_format_def_t *srcfmt)
{
    dst->data = src->data;
    dst->datalen = src->datalen;
    dst->cleanup = cleanup_ref;
    dst->next = (zebra_image_t*)src;
    ((zebra_image_t*)src)->refcnt++;
}

/* append neutral UV plane to grayscale image */
static void convert_uvp_append (zebra_image_t *dst,
                                const zebra_format_def_t *dstfmt,
                                const zebra_image_t *src,
                                const zebra_format_def_t *srcfmt)
{
    uv_roundup(dst, dstfmt);
    dst->datalen = uvp_size(dst, dstfmt) * 2;
    size_t n = dst->width * dst->height;
    dst->datalen += n;
    assert(src->datalen >= n);
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    memcpy((void*)dst->data, src->data, n);
    memset((void*)dst->data + n, 0x80, dst->datalen - n);
}

#if 0
/* rearrange Y and UV samples */
static void convert_yc_swap (zebra_image_t *dst,
                             const zebra_format_def_t *dstfmt,
                             const zebra_image_t *src,
                             const zebra_format_def_t *srcfmt)
{
    uv_roundup(dst, dstfmt);
    dst->datalen = (dst->width * dst->height) * 2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    uint32_t *dstp = dst->data;
    uint32_t *srcp = src->data;
    ssize_t i;
    for(i = dst->datalen / 4; i; i--) {
        /* NB this works for both LE and BE */
        register uint32_t src = *(srcp++);
        *(dstp++) = ((src & 0xff00ff00) >> 8) | ((src & 0x00ff00ff) << 8);
    }
}
#endif

/* interleave YUV planes into packed YUV */
static void convert_yuv_pack (zebra_image_t *dst,
                              const zebra_format_def_t *dstfmt,
                              const zebra_image_t *src,
                              const zebra_format_def_t *srcfmt)
{
    assert(0);
    uv_roundup(dst, dstfmt);
    dst->datalen = dst->width * dst->height + uvp_size(dst, dstfmt) * 2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    uint8_t *dstp = (void*)dst->data;

    size_t srcm = uvp_size(src, srcfmt);
    size_t srcn = src->width * src->height;
    assert(src->datalen >= srcn + 2 * srcn);
    uint8_t flags = dstfmt->p.yuv.packorder ^ srcfmt->p.yuv.packorder;
    uint8_t *srcy = (void*)src->data;
    const uint8_t *srcu, *srcv;
    if(flags & 1) {
        srcv = src->data + srcn;
        srcu = srcv + srcm;
    } else {
        srcu = src->data + srcn;
        srcv = srcu + srcm;
    }
    flags &= 2;

    unsigned srcl = src->width >> srcfmt->p.yuv.ysub2;
    unsigned xmask = (1 << srcfmt->p.yuv.xsub2) - 1;
    unsigned ymask = (1 << srcfmt->p.yuv.ysub2) - 1;
    unsigned x, y;
    signed u, v;
    for(y = 0; y < src->height; y++) {
        if(y & ymask) {
            srcu -= srcl;  srcv -= srcl;
        }
        for(x = 0; x < src->width; x += 2) {
            if(!(x & xmask)) {
                u = *(srcu++);  v = *(srcv++);
            }
            if(flags) {
                *(dstp++) = *(srcy++);  *(dstp++) = u;
                *(dstp++) = *(srcy++);  *(dstp++) = v;
            } else {
                *(dstp++) = u;  *(dstp++) = *(srcy++);
                *(dstp++) = v;  *(dstp++) = *(srcy++);
            }
        }
    }
}

/* join packed YUV samples into YUV planes
 * FIXME currently ignores color and grayscales the image
 */
static void convert_yuv_unpack (zebra_image_t *dst,
                                const zebra_format_def_t *dstfmt,
                                const zebra_image_t *src,
                                const zebra_format_def_t *srcfmt)
{
    uv_roundup(dst, dstfmt);
    size_t dstn = dst->width * dst->height;
    size_t dstm2 = 0;
    if(dstfmt->format != ZEBRA_FMT_GRAY)
        dstm2 = uvp_size(dst, dstfmt) * 2;
    dst->datalen = dstn + dstm2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    if(dstm2)
        memset((void*)dst->data + dstn, 0x80, dstm2);
    uint8_t *dsty = (void*)dst->data;

    assert(src->datalen >= (src->width * src->height +
                            uvp_size(src, srcfmt) * 2));
    uint8_t flags = srcfmt->p.yuv.packorder ^ dstfmt->p.yuv.packorder;
    flags &= 2;
    const uint8_t *srcp = src->data;
    if(flags)
        srcp++;

    ssize_t i;
    for(i = dstn / 2; i; i--) {
        *(dsty++) = *(srcp++);  srcp++;
        *(dsty++) = *(srcp++);  srcp++;
    }
}

/* resample and resize UV plane(s)
 * FIXME currently ignores color and grayscales the image
 */
static void convert_uvp_resample (zebra_image_t *dst,
                                  const zebra_format_def_t *dstfmt,
                                  const zebra_image_t *src,
                                  const zebra_format_def_t *srcfmt)
{
    assert(0);
    uv_roundup(dst, dstfmt);
    size_t dstn = dst->width * dst->height;
    size_t dstm = uvp_size(dst, dstfmt);
    dst->datalen = dstn + dstm * 2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    memcpy((void*)dst->data, src->data, dstn);
    memset((void*)dst->data + dstn, 0x80, dstm * 2);
}

/* rearrange interleaved UV componets */
static void convert_uv_resample (zebra_image_t *dst,
                                 const zebra_format_def_t *dstfmt,
                                 const zebra_image_t *src,
                                 const zebra_format_def_t *srcfmt)
{
    assert(0);
    uv_roundup(dst, dstfmt);
    ssize_t dstn = dst->width * dst->height;
    dst->datalen = dstn + uvp_size(dst, dstfmt) * 2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    uint8_t *dstp = (void*)dst->data;

    uint8_t flags = (srcfmt->p.yuv.packorder ^ dstfmt->p.yuv.packorder) & 1;
    const uint8_t *srcp = src->data;

    uint8_t y0, y1, u, v, tmp;
    ssize_t i;
    for(i = dstn / 2; i; i--) {
        if(!(srcfmt->p.yuv.packorder & 2)) {
            y0 = *(srcp++);  u = *(srcp++);
            y1 = *(srcp++);  v = *(srcp++);
        }
        else {
            u = *(srcp++);  y0 = *(srcp++);
            v = *(srcp++);  y1 = *(srcp++);
        }
        if(flags) {
            tmp = u;  u = v;  v = tmp;
        }
        if(!(dstfmt->p.yuv.packorder & 2)) {
            *(dstp++) = y0;  *(dstp++) = u;
            *(dstp++) = y1;  *(dstp++) = v;
        }
        else {
            *(dstp++) = u;  *(dstp++) = y0;
            *(dstp++) = v;  *(dstp++) = y1;
        }
    }
}

/* YUV planes to packed RGB
 * FIXME currently ignores color and grayscales the image
 */
static void convert_yuvp_to_rgb (zebra_image_t *dst,
                                 const zebra_format_def_t *dstfmt,
                                 const zebra_image_t *src,
                                 const zebra_format_def_t *srcfmt)
{
    dst->datalen = dst->width * dst->height * dstfmt->p.rgb.bpp;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    uint8_t *dstp = (void*)dst->data;

    int drbits = RGB_SIZE(dstfmt->p.rgb.red);
    int drbit0 = RGB_OFFSET(dstfmt->p.rgb.red);
    int dgbits = RGB_SIZE(dstfmt->p.rgb.green);
    int dgbit0 = RGB_OFFSET(dstfmt->p.rgb.green);
    int dbbits = RGB_SIZE(dstfmt->p.rgb.blue);
    int dbbit0 = RGB_OFFSET(dstfmt->p.rgb.blue);

    size_t srcm = uvp_size(src, srcfmt);
    size_t srcn = src->width * src->height;
    assert(src->datalen >= srcn + 2 * srcm);
    uint8_t *srcy = (void*)src->data;

    ssize_t i;
    for(i = srcn; i; i--) {
        /* FIXME color space? */
        unsigned y = *(srcy++);
        uint32_t p = (((y >> drbits) << drbit0) |
                      ((y >> dgbits) << dgbit0) |
                      ((y >> dbbits) << dbbit0));
        convert_write_rgb(dstp, p, dstfmt->p.rgb.bpp);
        dstp += dstfmt->p.rgb.bpp;
    }
}

/* packed RGB to YUV planes
 * FIXME currently ignores color and grayscales the image
 */
static void convert_rgb_to_yuvp (zebra_image_t *dst,
                                 const zebra_format_def_t *dstfmt,
                                 const zebra_image_t *src,
                                 const zebra_format_def_t *srcfmt)
{
    uv_roundup(dst, dstfmt);
    size_t dstn = dst->width * dst->height;
    size_t dstm2 = 0;
    if(dstfmt->format != ZEBRA_FMT_GRAY)
        dstm2 = uvp_size(dst, dstfmt) * 2;
    dst->datalen = dstn + dstm2;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    if(dstm2)
        memset((void*)dst->data + dstn, 0x80, dstm2);
    uint8_t *dsty = (void*)dst->data;

    assert(src->datalen >= (src->width * src->height * srcfmt->p.rgb.bpp));
    const uint8_t *srcp = src->data;

    int rbits = RGB_SIZE(srcfmt->p.rgb.red);
    int rbit0 = RGB_OFFSET(srcfmt->p.rgb.red);
    int gbits = RGB_SIZE(srcfmt->p.rgb.green);
    int gbit0 = RGB_OFFSET(srcfmt->p.rgb.green);
    int bbits = RGB_SIZE(srcfmt->p.rgb.blue);
    int bbit0 = RGB_OFFSET(srcfmt->p.rgb.blue);

    ssize_t i;
    for(i = dstn; i; i--) {
        uint8_t r, g, b;
        uint32_t p = convert_read_rgb(srcp, srcfmt->p.rgb.bpp);
        srcp += srcfmt->p.rgb.bpp;

        /* FIXME endianness? */
        r = ((p >> rbit0) << rbits) & 0xff;
        g = ((p >> gbit0) << gbits) & 0xff;
        b = ((p >> bbit0) << bbits) & 0xff;

        /* FIXME color space? */
        uint16_t y = ((77 * r + 150 * g + 29 * b) + 0x80) >> 8;
        *(dsty++) = y;
    }
}

/* packed YUV to packed RGB */
static void convert_yuv_to_rgb (zebra_image_t *dst,
                                const zebra_format_def_t *dstfmt,
                                const zebra_image_t *src,
                                const zebra_format_def_t *srcfmt)
{
    size_t dstn = dst->width * dst->height;
    dst->datalen = dstn * dstfmt->p.rgb.bpp;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    uint8_t *dstp = (void*)dst->data;

    int drbits = RGB_SIZE(dstfmt->p.rgb.red);
    int drbit0 = RGB_OFFSET(dstfmt->p.rgb.red);
    int dgbits = RGB_SIZE(dstfmt->p.rgb.green);
    int dgbit0 = RGB_OFFSET(dstfmt->p.rgb.green);
    int dbbits = RGB_SIZE(dstfmt->p.rgb.blue);
    int dbbit0 = RGB_OFFSET(dstfmt->p.rgb.blue);

    assert(src->datalen >= (src->width * src->height +
                            uvp_size(src, srcfmt) * 2));
    uint8_t flags = srcfmt->p.yuv.packorder ^ dstfmt->p.yuv.packorder;
    flags &= 2;
    const uint8_t *srcp = src->data;
    if(flags)
        srcp++;

    ssize_t i;
    for(i = dstn; i; i--) {
        uint8_t y = *(srcp++);
        srcp++;

        if(y <= 16)
            y = 0;
        else if(y >= 235)
            y = 255;
        else
            y = (uint16_t)(y - 16) * 255 / 219;

        uint32_t p = (((y >> drbits) << drbit0) |
                      ((y >> dgbits) << dgbit0) |
                      ((y >> dbbits) << dbbit0));
        convert_write_rgb(dstp, p, dstfmt->p.rgb.bpp);
        dstp += dstfmt->p.rgb.bpp;
    }
}

/* packed RGB to packed YUV */
static void convert_rgb_to_yuv (zebra_image_t *dst,
                                const zebra_format_def_t *dstfmt,
                                const zebra_image_t *src,
                                const zebra_format_def_t *srcfmt)
{
    assert(0);
}

#if 0
/* packed RGB to Y plane + packed UV */
static void convert_rgb_to_nv (zebra_image_t *dst,
                               const zebra_format_def_t *dstfmt,
                               const zebra_image_t *src,
                               const zebra_format_def_t *srcfmt)
{
    assert(0);
}
#endif

/* resample and resize packed RGB components */
static void convert_rgb_resample (zebra_image_t *dst,
                                  const zebra_format_def_t *dstfmt,
                                  const zebra_image_t *src,
                                  const zebra_format_def_t *srcfmt)
{
    size_t dstn = dst->width * dst->height;
    dst->datalen = dstn * dstfmt->p.rgb.bpp;
    dst->data = malloc(dst->datalen);
    if(!dst->data) return;
    uint8_t *dstp = (void*)dst->data;

    int drbits = RGB_SIZE(dstfmt->p.rgb.red);
    int drbit0 = RGB_OFFSET(dstfmt->p.rgb.red);
    int dgbits = RGB_SIZE(dstfmt->p.rgb.green);
    int dgbit0 = RGB_OFFSET(dstfmt->p.rgb.green);
    int dbbits = RGB_SIZE(dstfmt->p.rgb.blue);
    int dbbit0 = RGB_OFFSET(dstfmt->p.rgb.blue);

    assert(src->datalen >= (src->width * src->height * srcfmt->p.rgb.bpp));
    const uint8_t *srcp = src->data;

    int srbits = RGB_SIZE(srcfmt->p.rgb.red);
    int srbit0 = RGB_OFFSET(srcfmt->p.rgb.red);
    int sgbits = RGB_SIZE(srcfmt->p.rgb.green);
    int sgbit0 = RGB_OFFSET(srcfmt->p.rgb.green);
    int sbbits = RGB_SIZE(srcfmt->p.rgb.blue);
    int sbbit0 = RGB_OFFSET(srcfmt->p.rgb.blue);

    ssize_t i;
    for(i = dstn; i; i--) {
        uint32_t p;
        uint8_t r, g, b;
        p = convert_read_rgb(srcp, srcfmt->p.rgb.bpp);
        srcp += srcfmt->p.rgb.bpp;

        /* FIXME endianness? */
        r = ((p >> srbit0) << srbits) & 0xff;
        g = ((p >> sgbit0) << sgbits) & 0xff;
        b = ((p >> sbbit0) << sbbits) & 0xff;

        p = (((r >> drbits) << drbit0) |
             ((g >> dgbits) << dgbit0) |
             ((b >> dbbits) << dbbit0));

        convert_write_rgb(dstp, p, dstfmt->p.rgb.bpp);
        dstp += dstfmt->p.rgb.bpp;
    }
}

/* group conversion matrix */
static conversion_def_t conversions[][5] = {
    { /* *from* GRAY */
        {   0, convert_copy },           /* to GRAY */
        {   8, convert_uvp_append },     /* to YUV_PLANAR */
        {  24, convert_yuv_pack },       /* to YUV_PACKED */
        {  32, convert_yuvp_to_rgb },    /* to RGB_PACKED */
        {   8, convert_uvp_append },     /* to YUV_NV */
    },
    { /* from YUV_PLANAR */
        {   1, convert_copy },           /* to GRAY */
        {  48, convert_uvp_resample },   /* to YUV_PLANAR */
        {  64, convert_yuv_pack },       /* to YUV_PACKED */
        { 128, convert_yuvp_to_rgb },    /* to RGB_PACKED */
        {  40, convert_uvp_append },     /* to YUV_NV */
    },
    { /* from YUV_PACKED */
        {  24, convert_yuv_unpack },     /* to GRAY */
        {  52, convert_yuv_unpack },     /* to YUV_PLANAR */
        {  20, convert_uv_resample },    /* to YUV_PACKED */
        { 144, convert_yuv_to_rgb },     /* to RGB_PACKED */
        {  18, convert_yuv_unpack },     /* to YUV_NV */
    },
    { /* from RGB_PACKED */
        { 112, convert_rgb_to_yuvp },    /* to GRAY */
        { 160, convert_rgb_to_yuvp },    /* to YUV_PLANAR */
        { 144, convert_rgb_to_yuv },     /* to YUV_PACKED */
        { 120, convert_rgb_resample },   /* to RGB_PACKED */
        { 152, convert_rgb_to_yuvp },    /* to YUV_NV */
    },
    { /* from YUV_NV (FIXME treated as GRAY) */
        {   1, convert_copy },           /* to GRAY */
        {   8, convert_uvp_append },     /* to YUV_PLANAR */
        {  24, convert_yuv_pack },       /* to YUV_PACKED */
        {  32, convert_yuvp_to_rgb },    /* to RGB_PACKED */
        {   8, convert_uvp_append },     /* to YUV_NV */
    }
};

const zebra_format_def_t *_zebra_format_lookup (uint32_t fmt)
{
    const zebra_format_def_t *def = NULL;
    int i = 0;
    while(i < num_format_defs) {
        def = &format_defs[i];
        if(fmt == def->format)
            return(def);
        i = i * 2 + 1;
        if(fmt > def->format)
            i++;
    }
    return(NULL);
}

zebra_image_t *zebra_image_convert (const zebra_image_t *src,
                                    unsigned long fmt)
{
    zebra_image_t *dst = zebra_image_create();
    dst->format = fmt;
    dst->width = src->width;
    dst->height = src->height;
    if(src->format == fmt) {
        convert_copy(dst, NULL, src, NULL);
        return(dst);
    }

    const zebra_format_def_t *srcfmt = _zebra_format_lookup(src->format);
    const zebra_format_def_t *dstfmt = _zebra_format_lookup(dst->format);
    if(!srcfmt || !dstfmt)
        return(NULL);

    if(srcfmt->group == dstfmt->group &&
       srcfmt->p.cmp == dstfmt->p.cmp) {
        convert_copy(dst, NULL, src, NULL);
        return(dst);
    }

    conversion_handler_t *func =
        conversions[srcfmt->group][dstfmt->group].func;

    dst->cleanup = zebra_image_free_data;
    func(dst, dstfmt, src, srcfmt);
    return(dst);
}

static inline int has_format (uint32_t fmt,
                              const uint32_t *fmts)
{
    for(; *fmts; fmts++)
        if(*fmts == fmt)
            return(1);
    return(0);
}

/* select least cost conversion from src format to available dsts */
int _zebra_best_format (uint32_t src,
                        uint32_t *dst,
                        const uint32_t *dsts)
{
    if(has_format(src, dsts)) {
        zprintf(8, "shared format: %4.4s\n", (char*)&src);
        if(dst)
            *dst = src;
        return(0);
    }
    if(dst)
        *dst = 0;
    const zebra_format_def_t *srcfmt = _zebra_format_lookup(src);
    if(!srcfmt)
        return(-1);

    zprintf(8, "from %.4s(%08x) to", (char*)&src, src);
    unsigned min_cost = -1;
    for(; *dsts; dsts++) {
        const zebra_format_def_t *dstfmt = _zebra_format_lookup(*dsts);
        if(!dstfmt) {
            
            continue;
        }
        int cost;
        if(srcfmt->group == dstfmt->group &&
           srcfmt->p.cmp == dstfmt->p.cmp)
            cost = 0;
        else
            cost = conversions[srcfmt->group][dstfmt->group].cost;
        if(_zebra_verbosity >= 8)
        fprintf(stderr, " %.4s(%08x)=%d", (char*)dsts, *dsts, cost);
        if(min_cost > cost) {
            min_cost = cost;
            if(dst)
                *dst = *dsts;
        }
    }
    if(_zebra_verbosity >= 8)
        fprintf(stderr, "\n");
    return(min_cost);
}

int zebra_negotiate_format (zebra_video_t *vdo,
                            zebra_window_t *win)
{
    if(!vdo && !win)
        return(0);

    errinfo_t *errdst = (vdo) ? &vdo->err : &win->err;
    if(verify_format_sort())
        return(err_capture(errdst, SEV_FATAL, ZEBRA_ERR_INTERNAL, __func__,
                           "image format list is not sorted!?"));

    if((vdo && !vdo->formats) || (win && !win->formats))
        return(err_capture(errdst, SEV_ERROR, ZEBRA_ERR_UNSUPPORTED, __func__,
                           "no input or output formats available"));

    uint32_t y800[2] = { fourcc('Y','8','0','0'), 0 };
    uint32_t *srcs = (vdo) ? vdo->formats : y800;
    uint32_t *dsts = (win) ? win->formats : y800;

    unsigned min_cost = -1;
    uint32_t min_fmt = 0;
    const uint32_t *fmt;
    for(fmt = format_prefs; *fmt; fmt++) {
        /* only consider formats supported by video device */
        if(!has_format(*fmt, srcs))
            continue;
        uint32_t win_fmt = 0;
        int cost = _zebra_best_format(*fmt, &win_fmt, dsts);
        if(cost < 0) {
            zprintf(4, "%.4s(%08x) -> ? (unsupported)\n", (char*)fmt, *fmt);
            continue;
        }
        zprintf(4, "%.4s(%08x) -> %.4s(%08x) (%d)\n",
                (char*)fmt, *fmt, (char*)&win_fmt, win_fmt, cost);
        if(min_cost > cost) {
            min_cost = cost;
            min_fmt = *fmt;
            if(!cost)
                break;
        }
    }
    if(!min_fmt)
        return(err_capture(errdst, SEV_ERROR, ZEBRA_ERR_UNSUPPORTED, __func__,
                           "no supported image formats available"));
    if(!vdo)
        return(0);

    zprintf(2, "setting best format %.4s(%08x) (%d)\n",
            (char*)&min_fmt, min_fmt, min_cost);
    return(_zebra_video_init(vdo, min_fmt));
}
