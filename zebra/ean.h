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
#ifndef _EAN_H_
#define _EAN_H_

/* state of each parallel decode attempt */
typedef struct ean_pass_s {
    char state;                 /* module position of w[idx] in symbol */
#define STATE_ADDON 0x40        /*   scanning add-on */
#define STATE_IDX   0x1f        /*   element offset into symbol */
    char raw[7];                /* decode in process */
} ean_pass_t;

/* EAN/UPC specific decode state */
typedef struct ean_decoder_s {
    ean_pass_t pass[4];         /* state of each parallel decode attempt */
} ean_decoder_t;

/* reset EAN/UPC specific state */
static inline void ean_reset (ean_decoder_t *ean)
{
    ean->pass[0].state = -1;
    ean->pass[1].state = -1;
    ean->pass[2].state = -1;
    ean->pass[3].state = -1;
}

/* decode EAN/UPC symbols */
zebra_symbol_type_t zebra_decode_ean(zebra_decoder_t *dcode);

#endif
