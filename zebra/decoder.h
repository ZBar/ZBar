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
#ifndef _DECODER_H_
#define _DECODER_H_

#include <stdlib.h>     /* realloc */

#include <zebra.h>
#include "ean.h"
#include "code128.h"

/* size of bar width history (implementation assumes power of two) */
#ifndef DECODE_WINDOW
# define DECODE_WINDOW  8
#endif

/* initial data buffer allocation */
#ifndef BUFFER_MIN
# define BUFFER_MIN   0x20
#endif

/* maximum data buffer allocation
 * (longer symbols are rejected)
 */
#ifndef BUFFER_MAX
# define BUFFER_MAX  0x100
#endif

/* buffer allocation increment */
#ifndef BUFFER_INCR
# define BUFFER_INCR  0x10
#endif

/* symbology independent decoder state */
struct zebra_decoder_s {
    unsigned char idx;                  /* current width index */
    unsigned w[DECODE_WINDOW];          /* window of last N bar widths */
    zebra_symbol_type_t type;           /* type of last decoded data */
    unsigned lock : 1;                  /* buffer lock */

    /* everything above here is automatically reset */
    char *buf;                          /* decoded characters */
    unsigned buflen;                    /* dynamic buffer allocation */
    void *userdata;                     /* application data */
    zebra_decoder_handler_t *handler;   /* application callback */

    /* symbology specific state */
    ean_decoder_t ean;                  /* EAN/UPC parallel decode attempts */
    code128_decoder_t code128;          /* Code 128 decode state */
};

/* return current element color */
static inline char get_color (const zebra_decoder_t *dcode)
{
    return(dcode->idx & 1);
}

/* retrieve i-th previous element width */
static inline unsigned get_width(const zebra_decoder_t *dcode,
                                 unsigned char offset)
{
    return(dcode->w[(dcode->idx - offset) & (DECODE_WINDOW - 1)]);
}

/* calculate total character width "s"
 *   - start of character identified by context sensitive offset
 *     (<= DECODE_WINDOW - n)
 *   - size of character is n elements
 */
static inline unsigned calc_s (const zebra_decoder_t *dcode,
                               unsigned char offset,
                               unsigned char n)
{
    /* FIXME check that this gets unrolled for constant n */
    unsigned s = 0;
    while(n--)
        s += get_width(dcode, offset++);
    return(s);
}

/* fixed character width decode assist
 * bar+space width are compared as a fraction of the reference dimension "x"
 *   - +/- 1/2 x tolerance
 *   - measured total character width (s) compared to symbology baseline (n)
 *     (n = 7 for EAN/UPC, 11 for Code 128)
 *   - bar+space *pair width* "e" is used to factor out bad "exposures"
 *     ("blooming" or "swelling" of dark or light areas)
 *     => using like-edge measurements avoids these issues
 *   - n should be > 3
 */
static inline int decode_e (unsigned e,
                            unsigned s,
                            unsigned n)
{
    /* result is encoded number of units - 2
     * (for use as zero based index)
     * or -1 if invalid
     */
    unsigned char E = ((e * n * 2 + 1) / s - 3) / 2;
    return((E >= n - 3) ? -1 : E);
}

/* acquire shared state lock */
static inline char get_lock (zebra_decoder_t *dcode)
{
    if(dcode->lock)
        return(1);
    dcode->lock = 1;
    return(0);
}

/* ensure output buffer has sufficient allocation for request */
static inline char size_buf (zebra_decoder_t *dcode,
                             unsigned len)
{
    if(len < dcode->buflen)
        /* FIXME size reduction heuristic? */
        return(0);
    if(len > BUFFER_MAX)
        return(1);
    if(len < dcode->buflen + BUFFER_INCR) {
        len = dcode->buflen + BUFFER_INCR;
        if(len > BUFFER_MAX)
            len = BUFFER_MAX;
    }
    char *buf = realloc(dcode->buf, len);
    if(!buf)
        return(1);
    dcode->buf = buf;
    dcode->buflen = len;
    return(0);
}

#endif
