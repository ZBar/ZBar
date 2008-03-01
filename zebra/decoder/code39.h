/*------------------------------------------------------------------------
 *  Copyright 2008 (c) Jeff Brown <spadix@users.sourceforge.net>
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
#ifndef _CODE39_H_
#define _CODE39_H_

/* Code 39 specific decode state */
typedef struct code39_decoder_s {
    unsigned direction : 1;     /* scan direction: 0=fwd/space, 1=rev/bar */
    unsigned element : 4;       /* element offset 0-8 */
    int character : 12;         /* character position in symbol */
    unsigned w8;                /* one more history (FIXME hack) */
    unsigned width;             /* negedge width of last character */
} code39_decoder_t;

/* reset Code 39 specific state */
static inline void code39_reset (code39_decoder_t *dcode39)
{
    dcode39->direction = 0;
    dcode39->element = 0;
    dcode39->character = -1;
}

/* decode Code 39 symbols */
zebra_symbol_type_t zebra_decode_code39(zebra_decoder_t *dcode);

#endif
