/*------------------------------------------------------------------------
 *  Copyright 2007 (c) Jeff Brown <spadix@users.sourceforge.net>
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
#ifndef _ZEBRA_H_
#define _ZEBRA_H_

#ifdef __cplusplus
namespace zebra {
    extern "C" {
#endif


/* "color" of element: bar or space */
typedef enum zebra_color_e {
    ZEBRA_SPACE = 0,    /* light area or space between bars */
    ZEBRA_BAR = 1,      /* dark area or colored bar segment */
} zebra_color_t;

/* decoded symbol type */
typedef enum zebra_symbol_type_e {
    ZEBRA_NONE        =      0,   /* no symbol decoded */
    ZEBRA_PARTIAL     =      1,   /* intermediate status */
    ZEBRA_EAN8        =      8,   /* EAN-8 */
    ZEBRA_UPCE        =     11,   /* UPC-E */
    ZEBRA_UPCA        =     12,   /* UPC-A */
    ZEBRA_EAN13       =     13,   /* EAN-13 */
    ZEBRA_CODE128     =    128,   /* Code 128 */
    ZEBRA_SYMBOL      = 0x00ff,   /* mask for base symbol type */
    ZEBRA_ADDON2      = 0x0200,   /* 2-digit add-on flag */
    ZEBRA_ADDON5      = 0x0500,   /* 5-digit add-on flag */
    ZEBRA_ADDON       = 0x0700,   /* add-on flag mask */
} zebra_symbol_type_t;

/* return string name for symbol encoding */
extern const char *zebra_get_symbol_name(zebra_symbol_type_t sym);

/* return string name for addon encoding */
extern const char *zebra_get_addon_name(zebra_symbol_type_t sym);


/*------------------------------------------------------------*/
/* high-level bar width stream decoder interface
 * identifies symbols and extracts encoded data
 */

/* opaque decoder object */
struct zebra_decoder_s;
typedef struct zebra_decoder_s zebra_decoder_t;

/* decoder data handler callback function.
 * called by decoder when new data has just been decoded
 */
typedef void (zebra_decoder_handler_t)(zebra_decoder_t *decoder);

/* constructor */
extern zebra_decoder_t *zebra_decoder_create();

/* destructor */
extern void zebra_decoder_destroy(zebra_decoder_t *decoder);

/* clear all decoder state, including any partial symbols */
extern void zebra_decoder_reset(zebra_decoder_t *decoder);

/* mark start of a new scan pass,
 * clears any intra-symbol state and resets color to SPACE
 * any partially decoded symbol state is retained
 */
extern void zebra_decoder_new_scan(zebra_decoder_t *decoder);

/* process next bar/space width from input stream
 * width is in arbitrary relative units
 * first value of scan is SPACE width, alternating from there.
 * returns appropriate symbol type if width completes
 * decode of a symbol (data is available for retrieval)
 * returns ZEBRA_PARTIAL as a hint if part of a symbol was decoded
 * returns ZEBRA_NONE (0) if no new symbol data is available
 */
extern zebra_symbol_type_t zebra_decode_width(zebra_decoder_t *decoder,
                                              unsigned width);

/* return color of *next* element passed to zebra_decode_width() */
extern zebra_color_t zebra_decoder_get_color(const zebra_decoder_t *decoder);

/* returns last decoded data,
 * or NULL if no new data available.
 * data buffer is maintained by library,
 * contents are only valid between non-0 return from zebra_decode_width
 * and next library call
 */
extern const char *zebra_decoder_get_data(const zebra_decoder_t *decoder);

/* returns last decoded type,
 * or ZEBRA_NONE if no new data available
 */
extern zebra_symbol_type_t
zebra_decoder_get_type(const zebra_decoder_t *decoder);

/* setup data handler callback.
 * the registered function will be called by the decoder
 * just before zebra_decode_width returns a non-zero value.
 * pass a NULL value to disable callbacks.
 * returns the previously registered handler
 */
extern zebra_decoder_handler_t*
zebra_decoder_set_handler(zebra_decoder_t *decoder,
                          zebra_decoder_handler_t *handler);

/* associate user specified data value with the decoder */
extern void zebra_decoder_set_userdata(zebra_decoder_t *decoder,
                                       void *userdata);

/* return user specified data value associated with the decoder */
extern void *zebra_decoder_get_userdata(const zebra_decoder_t *decoder);


/*------------------------------------------------------------*/
/* low-level intensity sample stream scanner interface
 * identifies "bar" edges and measures width between them
 * optionally passes to bar width decoder
 */

/* opaque scanner object */
struct zebra_scanner_s;
typedef struct zebra_scanner_s zebra_scanner_t;

/* constructor
 * if decoder is non-NULL it will be attached to scanner
 * and called automatically at each new edge
 * current color is initialized to SPACE
 * (so an initial BAR->SPACE transition may be discarded)
 */
extern zebra_scanner_t *zebra_scanner_create(zebra_decoder_t *decoder);

/* destructor */
extern void zebra_scanner_destroy(zebra_scanner_t *scanner);

/* clear all scanner state.
 * also resets an associated decoder
 */
extern zebra_symbol_type_t zebra_scanner_reset(zebra_scanner_t *scanner);

/* mark start of a new scan pass, resets color to SPACE.
 * also updates an associated decoder
 */
extern zebra_symbol_type_t zebra_scanner_new_scan(zebra_scanner_t *scanner);

/* process next sample intensity value (y)
 * intensity is in arbitrary relative units
 * if decoder is attached, returns result of zebra_decode_width()
 * otherwise returns non-0 (ZEBRA_PARTIAL) when new edge is detected
 * and 0 (ZEBRA_NONE) if no new edge is detected
 */
extern zebra_symbol_type_t zebra_scan_y(zebra_scanner_t *scanner,
                                        int y);

/* process next sample from RGB (or BGR) triple */
static inline zebra_symbol_type_t zebra_scan_rgb24 (zebra_scanner_t *scanner,
                                                    unsigned char *rgb)
{
    return(zebra_scan_y(scanner, rgb[0] + rgb[1] + rgb[2]));
}

/* retrieve last scanned width */
extern unsigned zebra_scanner_get_width(const zebra_scanner_t *scanner);

/* retrieve last scanned color */
extern zebra_color_t zebra_scanner_get_color(const zebra_scanner_t *scanner);


/*------------------------------------------------------------*/
/* 2-D image walker interface
 * walks a default scan pattern over image data buffers
 * using configured image parameters
 */

/* opaque walker object */
struct zebra_img_walker_s;
typedef struct zebra_img_walker_s zebra_img_walker_t;

/* pixel handler callback function.
 * called by image walker for each pixel of the walk.
 * any non-zero return value will terminate the walk
 * with the returned value.
 */
typedef char (zebra_img_walker_handler_t)(zebra_img_walker_t *walker,
                                          void *pixel);

/* constructor
 * create a new image walker instance.  image parameters
 * must be initialized before a walk can be started
 */
extern zebra_img_walker_t *zebra_img_walker_create();

/* destructor */
extern void zebra_img_walker_destroy(zebra_img_walker_t *walker);

/* set the pixel handler */
extern void zebra_img_walker_set_handler(zebra_img_walker_t *walker,
                                         zebra_img_walker_handler_t *handler);

/* associate user specified data value with the walker */
extern void zebra_img_walker_set_userdata(zebra_img_walker_t *walker,
                                          void *userdata);

/* return user specified data value associated with the decoder */
extern void *zebra_img_walker_get_userdata(zebra_img_walker_t *walker);

/* set the width of the image in columns,
 * and the height of the image in rows
 */
extern void zebra_img_walker_set_size(zebra_img_walker_t *walker,
                                      unsigned width,
                                      unsigned height);


/* (optional) set column and row sizes.
 * bytes_per_col is the number of bytes in each column
 * of the image buffer (eg, often 1 for YUV format
 *  and 3 for packed RGB).  defaults to 1 if unspecified.
 * bytes_per_row is the number of bytes in each row
 * of the image buffer. defaults to width * bytes_per_col
 * if unspecified
 */
extern void zebra_img_walker_set_stride(zebra_img_walker_t *walker,
                                        unsigned bytes_per_col,
                                        unsigned bytes_per_row);

/* start walking over specified new image
 * returns the first non-zero value returned by the handler,
 * 0 when walk completes or -1 if walk cannot start
 * (usually because some image parameter is in error).
 * NB the image buffer is never dereferenced by the walker,
 * a NULL value *will* be walked (which could be used as an offset)
 */
extern char zebra_img_walk(zebra_img_walker_t *walker,
                           const void *image);

/* return current column location of walker */
extern unsigned zebra_img_walker_get_col(zebra_img_walker_t *walker);

/* return current row location of walker */
extern unsigned zebra_img_walker_get_row(zebra_img_walker_t *walker);


#ifdef __cplusplus
    }
}

# include "zebra/Decoder.h"
# include "zebra/Scanner.h"
# include "zebra/ImageWalker.h"
#endif

#endif
