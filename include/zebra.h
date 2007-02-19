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
    ZEBRA_NONE        =    0,   /* no symbol decoded */
    ZEBRA_PARTIAL     =    1,   /* intermediate status */
    ZEBRA_EAN8        =    8,   /* EAN-8 */
    ZEBRA_UPCE        =   11,   /* UPC-E */
    ZEBRA_UPCA        =   12,   /* UPC-A */
    ZEBRA_EAN13       =   13,   /* EAN-13 */
    ZEBRA_ADDON2      = 0x20,   /* 2-digit add-on flag */
    ZEBRA_ADDON5      = 0x50,   /* 5-digit add-on flag */
} zebra_symbol_type_t;


/*------------------------------------------------------------*/
/* high-level bar width stream decoder interface
 * identifies symbols and extracts encoded data
 */

/* opaque decoder object */
struct zebra_decoder_s;
typedef struct zebra_decoder_s zebra_decoder_t;

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

/* returns last decoded data,
 * or NULL if no new data available.
 * data buffer is maintained by library,
 * contents are only valid between non-0 return from zebra_decode_width
 * and next library call
 */
extern const char *zebra_decoder_get_data(const zebra_decoder_t *decoder);


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

extern void zebra_scanner_reset(zebra_scanner_t *scanner);
extern void zebra_scanner_new_scan(zebra_scanner_t *scanner);

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


#ifdef __cplusplus
  }
}

# include "zebra/Decoder.h"
# include "zebra/Scanner.h"
#endif

#endif
