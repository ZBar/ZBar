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
#ifdef DEBUG_EAN
# include <stdio.h>     /* fprintf */
#endif
#include <assert.h>

#include <zebra.h>
#include "decoder.h"

#ifdef DEBUG_EAN
# define dprintf(level, ...) \
    if(level <= DEBUG_EAN) \
        fprintf(stderr, __VA_ARGS__)
#else
# define dprintf(...)
#endif

/* partial decode symbol location */
typedef enum symbol_partial_e {
    RIGHT_HALF = 6,
    LEFT_HALF  = 7,
} symbol_partial_t;

/* convert compact encoded D2E1E2 to character (bit4 is parity) */
static const char digits[] = {  /* E1   E2 */
    0x06, 0x10, 0x04, 0x13,     /*  2  2-5 */
    0x19, 0x08, 0x11, 0x05,     /*  3  2-5 (d2 <= thr) */
    0x09, 0x12, 0x07, 0x15,     /*  4  2-5 (d2 <= thr) */
    0x16, 0x00, 0x14, 0x03,     /*  5  2-5 */
    0x18, 0x01, 0x02, 0x17,     /* E1E2=43,44,33,34 (d2 > thr) */
};

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

#ifdef DEBUG_EAN
static char debug_buf[0x18];

static inline const char *dsprintbuf(ean_decoder_t *ean)
{
    int i;
    for(i = 0; i < 7; i++)
        debug_buf[i] = ((ean->buf[0] < 0 || ean->buf[i] < 0)
                        ? '-'
                        : ean->buf[i] + '0');
    debug_buf[i] = ' ';
    for(; i < 13; i++)
        debug_buf[i + 1] = ((ean->buf[7] < 0 || ean->buf[i] < 0)
                            ? '-'
                            : ean->buf[i] + '0');
    debug_buf[i + 1] = ' ';
    for(; i < 18; i++)
        debug_buf[i + 2] = ((ean->buf[13] < 0 || ean->buf[i] < 0)
                            ? '-'
                            : ean->buf[i] + '0');
    debug_buf[i + 2] = '\0';
    return(debug_buf);
}
#endif

/* evaluate previous N (>= 2) widths as auxiliary pattern,
 * using preceding 4 as character width
 */
static inline unsigned char aux_end (zebra_decoder_t *dcode,
                                     unsigned char n)
{
    /* reference width from previous character */
    unsigned s = calc_s(dcode, n, 4);

    dprintf(2, " (");
    char code = 0;
    char i;
    for(i = 0; i < n - 1; i++) {
        unsigned e = get_width(dcode, i) + get_width(dcode, i + 1);
        dprintf(2, " %d", e);
        code = (code << 2) | decode_e(e, s, 7);
        if(code < 0)
            return(-1);
    }
    dprintf(2, ") s=%d aux=%x", s, code);
    return(code);
}

/* determine possible auxiliary pattern
 * using current 4 as possible character
 */
static inline char aux_start (zebra_decoder_t *dcode,
                              unsigned s)
{
    /* FIXME NB add-on has no guard in reverse */
    unsigned e2 = get_width(dcode, 5) + get_width(dcode, 6);
    if(decode_e(e2, s, 7)) {
        dprintf(2, " [invalid any]");
        return(/*FIXME (get_color(dcode) == ZEBRA_SPACE) ? STATE_ADDON : */-1);
    }

    unsigned e1 = get_width(dcode, 4) + get_width(dcode, 5);
    unsigned char E1 = decode_e(e1, s, 7);

    if(get_color(dcode) == ZEBRA_BAR) {
        /* check for quiet-zone */
        if((get_width(dcode, 7) * 14 + 1) / s >= 3) {
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
        unsigned e3 = get_width(dcode, 6) + get_width(dcode, 7);
        if(!decode_e(e3, s, 7)) {
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
    unsigned e1 = ((get_color(dcode) == ZEBRA_BAR)
                   ? get_width(dcode, 0) + get_width(dcode, 1)
                   : get_width(dcode, 2) + get_width(dcode, 3));
    unsigned e2 = get_width(dcode, 1) + get_width(dcode, 2);
    dprintf(2, "\n        e1=%d e2=%d", e1, e2);

    /* create compacted encoding for direct lookup */
    char code = (decode_e(e1, s, 7) << 2) | decode_e(e2, s, 7);
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
        unsigned d2 = ((get_color(dcode) == ZEBRA_BAR)
                       ? get_width(dcode, 0) + get_width(dcode, 2)
                       : get_width(dcode, 1) + get_width(dcode, 3));
        d2 *= 7;
        char mid = (((1 << code) & 0x0420)
                    ? 3     /* E1E2 in 33,44 */
                    : 4);   /* E1E2 in 34,43 */
        char alt = d2 > (mid * s);
        if(alt)
            code = ((code >> 1) & 3) | 0x10; /* compress code space */
        dprintf(2, " (d2=%d(%d) alt=%d)", d2, mid * s, alt);
    }
    dprintf(2, " char=%02x", digits[(unsigned char)code]);
    assert(code < 0x14);
    return(code);
}

static inline zebra_symbol_type_t partial_end (ean_pass_t *pass,
                                               char fwd)
{
    /* calculate parity index */
    char par = ((fwd)
                ? ((pass->raw[1] & 0x10) << 1 |
                   (pass->raw[2] & 0x10) |
                   (pass->raw[3] & 0x10) >> 1 |
                   (pass->raw[4] & 0x10) >> 2 |
                   (pass->raw[5] & 0x10) >> 3 |
                   (pass->raw[6] & 0x10) >> 4)
                : ((pass->raw[1] & 0x10) >> 4 |
                   (pass->raw[2] & 0x10) >> 3 |
                   (pass->raw[3] & 0x10) >> 2 |
                   (pass->raw[4] & 0x10) >> 1 |
                   (pass->raw[5] & 0x10) |
                   (pass->raw[6] & 0x10) << 1));

    /* lookup parity combination */
    pass->raw[0] = parity_decode[par >> 1];
    if(par & 1)
        pass->raw[0] >>= 4;
    pass->raw[0] &= 0xf;
    dprintf(2, " par=%02x(%x)", par, pass->raw[0]);

    if(pass->raw[0] == 0xf)
        /* invalid parity combination */
        return(ZEBRA_NONE);

    if(!par == fwd) {
        /* reverse sampled digits */
        unsigned char i;
        for(i = 1; i < 4; i++) {
            char tmp = pass->raw[i];
            pass->raw[i] = pass->raw[7 - i];
            pass->raw[7 - i] = tmp;
        }
    }

    if(!par)
        return(RIGHT_HALF);
    if(par & 0x20)
        return(LEFT_HALF);
    return(/*ZEBRA_UPCE*/ZEBRA_NONE);
}

/* update state for one of 4 parallel passes */
static inline zebra_symbol_type_t decode_pass (zebra_decoder_t *dcode,
                                               ean_pass_t *pass)
{
    pass->state++;
    char idx = pass->state & STATE_IDX;
    if((1 << idx) &
       ((pass->state & STATE_ADDON)
        ? 0x00108421
        : 0x00111111))
    {
        unsigned s = calc_s(dcode, 0, 4);
        if(!s)
            return(0);
        dprintf(2, " s=%d", s);
        /* validate guard bars before decoding first char of symbol */
        if(!pass->state) {
            pass->state = aux_start(dcode, s);
            if(pass->state < 0)
                return(0);
            idx = pass->state & STATE_IDX;
        }
        char code = decode4(dcode, s);
        if(code < 0)
            pass->state = -1;
        else {
            dprintf(2, "\n        raw[%x]=%02x =>", idx >> 2,
                    digits[(unsigned char)code]);
            pass->raw[(idx >> 2) + 1] = digits[(unsigned char)code];
            dprintf(2, " raw=%d%d%d%d%d%d%d",
                    pass->raw[0] & 0xf, pass->raw[1] & 0xf,
                    pass->raw[2] & 0xf, pass->raw[3] & 0xf,
                    pass->raw[4] & 0xf, pass->raw[5] & 0xf,
                    pass->raw[6] & 0xf);
        }
    }
    else if((get_color(dcode) == ZEBRA_BAR) &&
            ((pass->state & STATE_ADDON)
             ? idx == 0x06 || idx == 0x09 || idx == 0x16 || idx == 0x19
             : idx == 0x17 || idx == 0x18)) {
        char fwd = idx == 0x18;
        dprintf(2, " fwd=%x", fwd);
        zebra_symbol_type_t part = ZEBRA_NONE;
        if(!aux_end(dcode, (fwd) ? 4 : 3)) {
            part = partial_end(pass, fwd);
            dprintf(2, "\n");
            dprintf(1, "decode=%x%x%x%x%x%x%x(%x)\n",
                    pass->raw[0] & 0xf, pass->raw[1] & 0xf,
                    pass->raw[2] & 0xf, pass->raw[3] & 0xf,
                    pass->raw[4] & 0xf, pass->raw[5] & 0xf,
                    pass->raw[6] & 0xf, part);
        }
        else
            dprintf(2, " [invalid end guard]");
        pass->state = -1;
        return(part);
    }
    return(0);
}

static inline char check_ean13_parity (ean_decoder_t *ean)
{
    unsigned char chk = 0;
    unsigned char i;
    for(i = 0; i < 12; i++) {
        unsigned char d = ean->buf[i];
        assert(d < 10);
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
    unsigned char d = ean->buf[12];
    assert(d < 10);
    if(chk != d) {
        dprintf(1, "checksum mismatch %d != %d (%s)\n",
                chk, d, dsprintbuf(ean));
        return(-1);
    }
    return(0);
}

static inline zebra_symbol_type_t integrate_partial (ean_decoder_t *ean,
                                                     ean_pass_t *pass,
                                                     zebra_symbol_type_t part)
{
    /* FIXME UPC-E, EAN-8 */
    /* copy raw data into holding buffer */
    /* if same partial is not consistent, reset others */
    dprintf(2, " integrate part=%x (%s)", part, dsprintbuf(ean));
    unsigned char i;
    if(part == LEFT_HALF) {
        if(ean->buf[0] >= 0 && ean->buf[7] >= 0)
            part = ZEBRA_EAN13;

        for(i = 0; i < 7; i++) {
            char digit = pass->raw[i] & 0xf;
            if(part == ZEBRA_EAN13 &&
               ean->buf[i] != digit) {
                /* partial mismatch - reset other parts */
                ean->buf[7] = ean->buf[13] = -1;
                part = ZEBRA_PARTIAL;
            }
            ean->buf[i] = digit;
        }
        if(part == LEFT_HALF && ean->buf[7] >= 0)
            part = ZEBRA_EAN13;
    }
    else if(part == RIGHT_HALF) {
        if(ean->buf[0] >= 0 && ean->buf[7] >= 0)
            part = ZEBRA_EAN13;
        for(i = 0; i < 6; i++) {
            char digit = pass->raw[i + 1] & 0xf;
            if(part == ZEBRA_EAN13 &&
               ean->buf[i + 7] != digit) {
                /* partial mismatch - reset other parts */
                ean->buf[0] = ean->buf[13] = -1;
                part = ZEBRA_PARTIAL;
            }
            ean->buf[i + 7] = digit;
        }
        if(part == RIGHT_HALF && ean->buf[0] >= 0)
            part = ZEBRA_EAN13;
    }
    else {
        unsigned char len = (part == ZEBRA_ADDON5) ? 5 : 2;
        if(ean->buf[0] >= 0 && ean->buf[7] >= 0 && ean->buf[13] >= 0)
            part |= ZEBRA_EAN13;
        for(i = 0; i < len; i++) {
            char digit = pass->raw[i] & 0xf;
            if((part & ZEBRA_SYMBOL) == ZEBRA_EAN13 &&
               ean->buf[i + 13] != digit) {
                /* partial mismatch - reset other parts */
                ean->buf[0] = ean->buf[7] = -1;
                part &= ZEBRA_ADDON;
            }
            ean->buf[i + 13] = digit;
        }
    }
    if(part == ZEBRA_EAN13 &&
       check_ean13_parity(ean))
        /* invalid parity */
        part = ZEBRA_NONE;

    if(part == ZEBRA_EAN13 && ean->buf[13] >= 0)
        part |= (ean->buf[15] >= 0) ? ZEBRA_ADDON5 : ZEBRA_ADDON2;

    if(part == LEFT_HALF || part == RIGHT_HALF)
        part = ZEBRA_PARTIAL;
    return(part);
}

/* copy result to output buffer */
static inline void postprocess (zebra_decoder_t *dcode,
                                zebra_symbol_type_t sym)
{
    ean_decoder_t *ean = &dcode->ean;
    int i = 0;
    if((sym & ZEBRA_SYMBOL) > ZEBRA_PARTIAL)
        for(; i < 12 && ean->buf[i] >= 0; i++)
            dcode->buf[i] = ean->buf[i] + '0';
    int j = i;
    if(sym & ZEBRA_ADDON)
        for(i = 13; ean->buf[i] >= 0; i++)
            dcode->buf[j++] = ean->buf[i] + '0';
    dcode->buf[j] = '\0';
}

zebra_symbol_type_t zebra_decode_ean (zebra_decoder_t *dcode)
{
    /* process upto 4 separate passes */
    zebra_symbol_type_t sym = ZEBRA_NONE;
    unsigned char pass_idx = dcode->idx & 3;

    unsigned char i;
    for(i = 0; i < 4; i++) {
        ean_pass_t *pass = &dcode->ean.pass[i];
        if(pass->state >= 0 ||
           i == pass_idx)
        {
            dprintf(2, "      ean[%x/%x]: idx=%x st=%d",
                    pass_idx, i, dcode->idx, pass->state);
            zebra_symbol_type_t part = decode_pass(dcode, pass);
            if(part) {
                /* update accumulated data from new partial decode */
                sym = integrate_partial(&dcode->ean, pass, part);
                if(sym) {
                    /* this pass valid => _reset_ all passes */
                    dprintf(2, " sym=%x", sym);
                    ean_new_scan(&dcode->ean);
                    if(sym > ZEBRA_PARTIAL) {
                        if(!get_lock(dcode))
                            postprocess(dcode, sym);
                        else {
                            dprintf(1, " [locked]");
                            sym = ZEBRA_PARTIAL;
                        }
                    }
                }
            }
            dprintf(2, "\n");
        }
    }
    return(sym);
}
