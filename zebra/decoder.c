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

#include <config.h>
#ifdef DEBUG_DECODER
# include <stdio.h>     /* fprintf */
#endif
#include <stdlib.h>     /* malloc, free */
#include <string.h>     /* memset */
#include <assert.h>

#include "zebra.h"

#ifdef DEBUG_DECODER
# define dprintf(level, ...) \
    if(level <= DEBUG_DECODER) \
        fprintf(stderr, __VA_ARGS__)
#else
# define dprintf(...)
#endif

/* convert compact encoded D2E1E2 to character (bit4 is parity) */
static const char digits[] = {  /* E1   E2 */
    0x06, 0x10, 0x04, 0x13,     /*  2  2-5 */
    0x19, 0x08, 0x11, 0x05,     /*  3  2-5 (d2 <= thr) */
    0x09, 0x12, 0x07, 0x15,     /*  4  2-5 (d2 <= thr) */
    0x16, 0x00, 0x14, 0x03,     /*  5  2-5 */
    0x18, 0x01, 0x02, 0x17,     /* E1E2=43,44,33,34 (d2 > thr) */
};

/* partial decode symbol location */
typedef enum symbol_partial_e {
    RIGHT_HALF = 6,
    LEFT_HALF  = 7,
} symbol_partial_t;

static const char parity_decode[] = {
    0xf0, /* [xx] BBBBBB = RIGHT half EAN-13 */

    /* UPC-E check digit encoding */
    0xff,
    0xff,
    0x0f, /* [07] BBBAAA = 0 */
    0xff,
    0x1f, /* [0b] BBABAA = 1 */
    0x2f, /* [0d] BBAABA = 2 */
    0xf3, /* [0e] BBAAAB = 3 */
    0xff,
    0x4f, /* [13] BABBAA = 4 */
    0x7f, /* [15] BABABA = 7 */
    0xf8, /* [16] BABAAB = 8 */
    0x5f, /* [19] BAABBA = 5 */
    0xf9, /* [1a] BAABAB = 9 */
    0xf6, /* [1c] BAAABB = 6 */
    0xff,

    /* LEFT half EAN-13 leading digit */
    0xff,
    0x6f, /* [23] ABBBAA = 6 */
    0x9f, /* [25] ABBABA = 9 */
    0xf5, /* [26] ABBAAB = 5 */
    0x8f, /* [29] ABABBA = 8 */
    0xf7, /* [2a] ABABAB = 7 */
    0xf4, /* [2c] ABAABB = 4 */
    0xff,
    0x3f, /* [31] AABBBA = 3 */
    0xf2, /* [32] AABBAB = 2 */
    0xf1, /* [34] AABABB = 1 */
    0xff,
    0xff,
    0xff,
    0xff,
    0x0f, /* [3f] AAAAAA = 0 */
};


typedef struct scan_s {
    char state;                 /* module position of w[idx] in symbol */
#define STATE_ADDON 0x40        /*   scanning add-on */
#define STATE_IDX   0x1f        /*   element offset into symbol */
    char raw[7];                /* decode in process */
} scan_t;

/* decoder state */
struct zebra_decoder_s {
    unsigned char idx;                  /* current width index */
    unsigned w[8];                      /* window of last N bar widths */
    scan_t scans[4];                    /* track decode states in parallel */
    char buf[20];                       /* decoded characters */
    zebra_symbol_type_t type;           /* type of last decoded data */
    void *userdata;                     /* application data */
    zebra_decoder_handler_t *handler;   /* application callback */
};

zebra_decoder_t *zebra_decoder_create ()
{
    zebra_decoder_t *dcode = malloc(sizeof(zebra_decoder_t));
    zebra_decoder_reset(dcode);
    return(dcode);
}

void zebra_decoder_destroy (zebra_decoder_t *dcode)
{
    free(dcode);
}

void zebra_decoder_reset (zebra_decoder_t *dcode)
{
    memset(dcode, 0, (long)&dcode->userdata - (long)dcode);
    dcode->scans[0].state = -1;
    dcode->scans[1].state = -1;
    dcode->scans[2].state = -1;
    dcode->scans[3].state = -1;
}

void zebra_decoder_new_scan (zebra_decoder_t *dcode)
{
    /* soft reset decoder */
    memset(dcode->w, 0, sizeof(dcode->w));
    dcode->idx = 0;
    dcode->scans[0].state = -1;
    dcode->scans[1].state = -1;
    dcode->scans[2].state = -1;
    dcode->scans[3].state = -1;
}

const char *zebra_decoder_get_data (const zebra_decoder_t *dcode)
{
    return(dcode->buf);
}

zebra_decoder_handler_t *
zebra_decoder_set_handler (zebra_decoder_t *dcode,
                           zebra_decoder_handler_t handler)
{
    zebra_decoder_handler_t *result = dcode->handler;
    dcode->handler = handler;
    return(result);
}

void zebra_decoder_set_userdata (zebra_decoder_t *dcode,
                                 void *userdata)
{
    dcode->userdata = userdata;
}

void *zebra_decoder_get_userdata (zebra_decoder_t *dcode)
{
    return(dcode->userdata);
}

zebra_symbol_type_t zebra_decoder_get_type (const zebra_decoder_t *dcode)
{
    return((dcode->type == RIGHT_HALF ||
            dcode->type == LEFT_HALF)
           ? ZEBRA_PARTIAL
           : dcode->type);
}

const char *zebra_get_symbol_name (zebra_symbol_type_t sym)
{
    switch(sym & ZEBRA_SYMBOL) {
    case ZEBRA_EAN8: return("EAN8");
    case ZEBRA_UPCE: return("UPC-E");
    case ZEBRA_UPCA: return("UPC-A");
    case ZEBRA_EAN13: return("EAN-13");
    default: return("UNKNOWN");
    }
}

const char *zebra_get_addon_name (zebra_symbol_type_t sym)
{
    switch(sym & ZEBRA_ADDON) {
    case ZEBRA_ADDON2: return("+2");
    case ZEBRA_ADDON5: return("+5");
    default: return("");
    }
}


/* bar+space width are compared as a fraction of the reference dimension "x"
 *   - +/- 1/2 x tolerance
 *   - total character width (s) is always 7 "units" for EAN/UPC
 *     => nominal "x" is 1/7th of "s"
 *   - bar+space *pair width* "e" is used to factor out bad "exposures"
 *     ("blooming" or "swelling" of dark or light areas)
 *     => using like-edge measurements avoids these issues
 */
#define ZEBRA_E_FIXED 6
#define ZEBRA_E(e) (((e) << ZEBRA_E_FIXED) / 14)

static inline char decode_e (unsigned e,
                             unsigned s)
{
    e = (e * 14 + 1) / s;
    if(e < 7) {
        if(e < 3) return(-1);   /* invalid */
        if(e < 5) return(0);    /* Ei = 2 (00) */
        else      return(1);    /* Ei = 3 (01) */
    }
    if(e >= 11)   return(-1);   /* invalid */
    if(e < 9)     return(2);    /* Ei = 4 (10) */
    else          return(3);    /* Ei = 5 (11) */
}

/* retrieve i-th previous element width */
static inline unsigned width(zebra_decoder_t *dcode,
                             unsigned char offset)
{
    return(dcode->w[(dcode->idx - offset) & 7]);
}

/* calculate total character width "s"
 *   => start of character identified by context sensitive offset (<= 4)
 */
static inline unsigned calc_s (zebra_decoder_t *dcode,
                               unsigned char offset)
{
    return(width(dcode, offset) +
           width(dcode, offset + 1) +
           width(dcode, offset + 2) +
           width(dcode, offset + 3));
}

/* evaluate previous N (>= 2) widths as auxiliary pattern,
 * using preceding 4 as character width
 */
static inline unsigned char aux_end (zebra_decoder_t *dcode,
                                     unsigned char n)
{
    /* reference width from previous character */
    unsigned s = calc_s(dcode, n);

    char code = 0;
    char i;
    for(i = 0; i < n - 1; i++) {
        unsigned e = width(dcode, i) + width(dcode, i + 1);
        code = (code << 2) | decode_e(e, s);
        if(code < 0)
            return(-1);
    }
    dprintf(2, " aux=%x", code);
    return(code);
}

/* determine possible auxiliary pattern
 * using current 4 as possible character
 */
static inline char aux_start (zebra_decoder_t *dcode,
                              unsigned s)
{
    /* FIXME NB add-on has no guard in reverse */
    unsigned e2 = width(dcode, 5) + width(dcode, 6);
    if(decode_e(e2, s)) {
        dprintf(2, " [invalid any]");
        return(/*FIXME ((dcode->idx & 1) == ZEBRA_SPACE) ? STATE_ADDON : */-1);
    }

    unsigned e1 = width(dcode, 4) + width(dcode, 5);
    unsigned char E1 = decode_e(e1, s);

    if((dcode->idx & 1) == ZEBRA_BAR) {
        /* check for quiet-zone */
        if((width(dcode, 7) * 14 + 1) / s >= 3) {
            if(!E1) {
                dprintf(2, " [valid normal]");
                return(0); /* normal symbol start */
            }
            else if(E1 == 1) {
                dprintf(2, " [valid add-on]");
                return(STATE_ADDON); /* add-on symbol start */
            }
        }
        dprintf(2, " [invalid start]");
        return(-1);
    }

    if(!E1) {
        /* attempting decode from SPACE => validate center guard */
        unsigned e3 = width(dcode, 6) + width(dcode, 7);
        if(!decode_e(e3, s)) {
            dprintf(2, " [valid center]");
            return(0); /* start after center guard */
        }
    }
    dprintf(2, " [invalid center]");
    return(/*STATE_ADDON*/-1);
}

/* attempt to decode previous 4 widths (2 bars and 2 spaces) as a character */
static inline char decode4 (zebra_decoder_t *dcode,
                            unsigned s)
{
    /* calculate similar edge measurements */
    unsigned e1 = (((dcode->idx & 1) == ZEBRA_BAR)
                   ? width(dcode, 0) + width(dcode, 1)
                   : width(dcode, 2) + width(dcode, 3));
    unsigned e2 = width(dcode, 1) + width(dcode, 2);
    dprintf(2, "\n        e1=%d e2=%d", e1, e2);

    /* create compacted encoding for direct lookup */
    char code = (decode_e(e1, s) << 2) | decode_e(e2, s);
    if(code < 0)
        return(-1);
    dprintf(2, " code=%x", code);

    /* 4 combinations require additional determinant (D2)
       E1E2 == 34 (0110)
       E1E2 == 43 (1001)
       E1E2 == 33 (0101)
       E1E2 == 44 (1010)
     */
    if((1 << code) & 0x0660) {
        /* use sum of bar widths */
        unsigned d2 = (((dcode->idx & 1) == ZEBRA_BAR)
                       ? width(dcode, 0) + width(dcode, 2)
                       : width(dcode, 1) + width(dcode, 3));
        d2 = ((unsigned long)d2 << ZEBRA_E_FIXED) / s;
        char alt = (((1 << code) & 0x0420)
                    ? d2 > ZEBRA_E(6)   /* E1E2 in 33,44 */
                    : d2 > ZEBRA_E(8)); /* E1E2 in 34,43 */
        if(alt)
            code = ((code >> 1) & 3) | 0x10; /* compress code space */
        dprintf(2, " (d2=%d alt=%d)", d2, alt);
    }
    dprintf(2, " char=%02x", digits[(unsigned char)code]);
    assert(code < 0x14);
    return(code);
}

static inline zebra_symbol_type_t partial_end (scan_t *scan, char fwd)
{
    /* calculate parity index */
    char par = ((fwd)
                ? ((scan->raw[1] & 0x10) << 1 |
                   (scan->raw[2] & 0x10) |
                   (scan->raw[3] & 0x10) >> 1 |
                   (scan->raw[4] & 0x10) >> 2 |
                   (scan->raw[5] & 0x10) >> 3 |
                   (scan->raw[6] & 0x10) >> 4)
                : ((scan->raw[1] & 0x10) >> 4 |
                   (scan->raw[2] & 0x10) >> 3 |
                   (scan->raw[3] & 0x10) >> 2 |
                   (scan->raw[4] & 0x10) >> 1 |
                   (scan->raw[5] & 0x10) |
                   (scan->raw[6] & 0x10) << 1));

    /* lookup parity combination */
    scan->raw[0] = parity_decode[par >> 1];
    if(par & 1)
        scan->raw[0] >>= 4;
    scan->raw[0] &= 0xf;
    dprintf(2, " par=%02x(%x)", par, scan->raw[0]);

    if(scan->raw[0] == 0xf)
        /* invalid parity combination */
        return(ZEBRA_NONE);

    if(!par == fwd) {
        /* reverse sampled digits */
        unsigned char i;
        for(i = 1; i < 4; i++) {
            char tmp = scan->raw[i];
            scan->raw[i] = scan->raw[7 - i];
            scan->raw[7 - i] = tmp;
        }
    }

    if(!par)
        return(RIGHT_HALF);
    if(par & 0x20)
        return(LEFT_HALF);
    return(/*ZEBRA_UPCE*/ZEBRA_NONE);
}

/* update state for one of 4 parallel scans */
static inline zebra_symbol_type_t decode_scan (zebra_decoder_t *dcode,
                                               scan_t *scan)
{
    scan->state++;
    char idx = scan->state & STATE_IDX;
    if((1 << idx) &
       ((scan->state & STATE_ADDON)
        ? 0x00108421
        : 0x00111111))
    {
        unsigned s = calc_s(dcode, 0);
        if(!s)
            return(0);
        dprintf(2, " s=%d", s);
        /* validate guard bars before decoding first char of symbol */
        if(!scan->state) {
            scan->state = aux_start(dcode, s);
            if(scan->state < 0)
                return(0);
            idx = scan->state & STATE_IDX;
        }
        char code = decode4(dcode, s);
        if(code < 0)
            scan->state = -1;
        else {
            dprintf(2, "\n        raw[%x]=%02x =>", idx >> 2,
                    digits[(unsigned char)code]);
            scan->raw[(idx >> 2) + 1] = digits[(unsigned char)code];
            dprintf(2, " raw=%d%d%d%d%d%d%d",
                    scan->raw[0] & 0xf, scan->raw[1] & 0xf,
                    scan->raw[2] & 0xf, scan->raw[3] & 0xf,
                    scan->raw[4] & 0xf, scan->raw[5] & 0xf,
                    scan->raw[6] & 0xf);
        }
    }
    else if(((dcode->idx & 1) == ZEBRA_BAR) &&
            ((scan->state & STATE_ADDON)
             ? idx == 0x06 || idx == 0x09 || idx == 0x16 || idx == 0x19
             : idx == 0x17 || idx == 0x18)) {
        char fwd = idx == 0x18;
        dprintf(2, " fwd=%x", fwd);
        zebra_symbol_type_t part = ZEBRA_NONE;
        if(!aux_end(dcode, (fwd) ? 4 : 3)) {
            part = partial_end(scan, fwd);
            dprintf(2, "\n");
            dprintf(1, "decode=%x%x%x%x%x%x%x(%x)\n",
                    scan->raw[0] & 0xf, scan->raw[1] & 0xf,
                    scan->raw[2] & 0xf, scan->raw[3] & 0xf,
                    scan->raw[4] & 0xf, scan->raw[5] & 0xf,
                    scan->raw[6] & 0xf, part);
        }
        else
            dprintf(2, " [invalid end guard]");
        scan->state = -1;
        return(part);
    }
    return(0);
}

static inline char check_ean13_parity (zebra_decoder_t *dcode)
{
    unsigned char chk = 0;
    unsigned char i;
    for(i = 0; i < 12; i++) {
        unsigned char d = dcode->buf[i] - '0';
        chk += d;
        if(i & 1) {
            chk += d << 1;
            if(chk >= 20)
                chk -= 20;
        }
        if(chk >= 10)
            chk -= 10;
    }
    assert(chk < 10);
    if(chk)
        chk = 10 - chk;
    unsigned char d = dcode->buf[12] - '0';
    assert(d < 10);
    if(chk != d) {
        dprintf(1, "checksum mismatch %d != %d (%s)\n", chk, d, dcode->buf);
        return(-1);
    }
    return(0);
}

static inline zebra_symbol_type_t integrate_partial (zebra_decoder_t *dcode,
                                                     scan_t *scan,
                                                     zebra_symbol_type_t part)
{
    /* FIXME if same partial is not consistent, reset others */
    /* FIXME UPC-E, EAN-8 */
    unsigned char start;
    unsigned char len = 7;
    unsigned char i = 0;
    if(part == LEFT_HALF) {
        start = 0;
        if(dcode->buf[7])
            part = ZEBRA_EAN13;
    }
    else if(part == RIGHT_HALF) {
        i = 1;
        start = 7;
        if(dcode->buf[0])
            part = ZEBRA_EAN13;
    }
    else {
        start = 13;
        len = (part == ZEBRA_ADDON5) ? 5 : 2;
    }
    if(part == ZEBRA_EAN13 && dcode->buf[13])
        part |= (dcode->buf[15]) ? ZEBRA_ADDON5 : ZEBRA_ADDON2;

    /* copy raw data into output buffer */
    for(; i < len; i++, start++)
        dcode->buf[start] = (scan->raw[i] & 0xf) + '0';

    if(part == ZEBRA_EAN13 &&
       start <= 13 &&
       check_ean13_parity(dcode))
        /* invalid parity */
        part = ZEBRA_NONE;

    return(part);
}

zebra_symbol_type_t zebra_decode_width (zebra_decoder_t *dcode,
                                        unsigned w)
{
    dcode->w[dcode->idx & 7] = w;

    /* process upto 4 separate scans */
    zebra_symbol_type_t sym = ZEBRA_NONE;
    unsigned char scan = dcode->idx & 3;

    unsigned char i;
    for(i = 0; i < 4; i++)
        if(dcode->scans[i].state >= 0 ||
           i == scan)
        {
            dprintf(2, "    decode[%x/%x]: idx=%x st=%d w=%d",
                    scan, i, dcode->idx, dcode->scans[i].state, w);
            zebra_symbol_type_t part = decode_scan(dcode, &dcode->scans[i]);
            if(part) {
                /* update accumulated data from new partial decode */
                sym = integrate_partial(dcode, &dcode->scans[i], part);
                dcode->type = sym;
                if(sym) {
                    /* this scan valid => _reset_ all scans */
                    dcode->scans[0].state = -1;
                    dcode->scans[1].state = -1;
                    dcode->scans[2].state = -1;
                    dcode->scans[3].state = -1;
                }
            }
            dprintf(2, "\n");
        }
    dcode->idx++;
    if(sym == LEFT_HALF || sym == RIGHT_HALF)
        sym = ZEBRA_PARTIAL;
    if(sym && dcode->handler)
        dcode->handler(dcode);
    return(sym);
}
