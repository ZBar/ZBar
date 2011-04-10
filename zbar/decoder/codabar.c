/*------------------------------------------------------------------------
 *  Copyright 2011 (c) Jeff Brown <spadix@users.sourceforge.net>
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
#include <string.h>     /* memmove */

#include <zbar.h>

#ifdef DEBUG_CODABAR
# define DEBUG_LEVEL (DEBUG_CODABAR)
#endif
#include "debug.h"
#include "decoder.h"

#define NIBUF 6 /* initial scan buffer size */

static const signed char codabar_lo[12] = {
    0x0, 0x1, 0x4, 0x5, 0x2, 0xa, 0xb, 0x9,
    0x6, 0x7, 0x8, 0x3
};

static const unsigned char codabar_hi[8] = {
    0x1, 0x4, 0x7, 0x6, 0x2, 0x3, 0x0, 0x5
};

static const unsigned char codabar_characters[20] =
    "0123456789-$:/.+ABCD";

static inline int
check_width (unsigned ref,
             unsigned w)
{
    unsigned dref = ref;
    ref *= 4;
    w *= 4;
    return(ref - dref <= w && w <= ref + dref);
}

static inline signed char
codabar_decode7 (zbar_decoder_t *dcode)
{
    codabar_decoder_t *codabar = &dcode->codabar;
    unsigned s = codabar->s7;
    dbprintf(2, " s=%d", s);
    if(s < 7)
        return(-1);

    /* check width */
    if(!check_width(codabar->width, s)) {
        dbprintf(2, " [width]");
        return(-1);
    }

    /* extract min/max bar */
    unsigned ibar = decode_sortn(dcode, 4, 1);
    dbprintf(2, " bar=%04x", ibar);

    unsigned wbmax = get_width(dcode, ibar & 0xf);
    unsigned wbmin = get_width(dcode, ibar >> 12);
    if(8 * wbmin < wbmax ||
       8 * wbmin > 5 * wbmax)
    {
        dbprintf(2, " [bar ratio]");
        return(-1);
    }

    if(8 * get_width(dcode, (ibar >> 4) & 0xf) < 5 * wbmax)
        // single wide bar combinations
        ibar = (ibar >> 1) & 0x3;
    else if(8 * get_width(dcode, (ibar >> 8) & 0xf) >= 5 * wbmax)
        // three wide bars, no wide spaces
        ibar = (ibar >> 13) + 4;
    else {
        dbprintf(2, " [bar comb]");
        return(-1);
    }

    unsigned ispc = decode_sort3(dcode, 2);
    dbprintf(2, "(%x) spc=%03x", ibar, ispc);

    unsigned wsmax = get_width(dcode, ispc & 0xf);
    unsigned wsmid = get_width(dcode, (ispc >> 4) & 0xf);
    unsigned wsmin = get_width(dcode, (ispc >> 8) & 0xf);
    if(ibar >> 2) {
        // verify no wide spaces
        if(wsmin * 8 < wsmax * 3) {
            dbprintf(2, " [space comb0]");
            return(-1);
        }
        ibar &= 0x3;
        if(codabar->direction)
            ibar = 3 - ibar;
        int c = (0xfcde >> (ibar << 2)) & 0xf;
        dbprintf(2, " ex[%d]=%x", ibar, c);
        return(c);
    }
    else if(8 * wsmin > 5 * wsmax) {
        dbprintf(2, " [space comb1]");
        return(-1);
    }
    else if(8 * wsmid < 5 * wsmax) {
        // single wide space
        ispc = ((ispc & 0xf) >> 1) - 1;
        unsigned ic = (ispc << 2) | ibar;
        if(codabar->direction)
            ic = 11 - ic;
        int c = codabar_lo[ic];
        dbprintf(2, "(%d) lo[%d]=%x", ispc, ic, c);
        return(c);
    }
    else if(8 * wsmin < 5 * wsmid) {
        // two wide spaces, check start/stop
        ispc >>= 10;
        dbprintf(2, "(%d)", ispc);
        unsigned ic = ispc * 4 + ibar;
        zassert(ic < 8, -1, "ic=%d ispc=%d ibar=%d", ic, ispc, ibar);
        unsigned char c = codabar_hi[ic];
        if(c >> 2 != codabar->direction) {
            dbprintf(2, " [invalid stop]");
            return(-1);
        }
        c = (c & 0x3) | 0x10;
        dbprintf(2, " hi[%d]=%x", ic, c);
        return(c);
    }
    else {
        dbprintf(2, " [space ratio]");
        return(-1);
    }
}

static inline signed char
codabar_decode_start (zbar_decoder_t *dcode)
{
    codabar_decoder_t *codabar = &dcode->codabar;
    unsigned s = codabar->s7;
    if(s < 8)
        return(ZBAR_NONE);
    dbprintf(2, "      codabar: s=%d", s);

    /* check leading quiet zone - spec is 10x */
    unsigned qz = get_width(dcode, 8);
    if((qz && qz * 2 < s) ||
       4 * get_width(dcode, 0) > 3 * s)
    {
        dbprintf(2, " [invalid qz/ics]\n");
        return(ZBAR_NONE);
    }

    /* check space ratios first */
    unsigned ispc = decode_sort3(dcode, 2);
    dbprintf(2, " spc=%03x", ispc);
    if((ispc >> 8) == 4) {
        dbprintf(2, " [space comb]\n");
        return(ZBAR_NONE);
    }

    /* require 2 wide and 1 narrow spaces */
    unsigned wsmax = get_width(dcode, ispc & 0xf);
    unsigned wsmid = get_width(dcode, (ispc >> 4) & 0xf);
    unsigned wsmin = get_width(dcode, ispc >> 8);
    if(8 * wsmin < wsmax ||
       8 * wsmin > 5 * wsmax ||
       8 * wsmid < 5 * wsmax)
    {
        dbprintf(2, " [space ratio]\n");
        return(ZBAR_NONE);
    }
    ispc >>= 10;
    dbprintf(2, "(%d)", ispc);

    /* check bar ratios */
    unsigned ibar = decode_sortn(dcode, 4, 1);
    dbprintf(2, " bar=%04x", ibar);

    /* require 1 wide & 3 narrow bars */
    unsigned wbmax = get_width(dcode, ibar & 0xf);
    unsigned wbmin = get_width(dcode, ibar >> 12);
    if(8 * wbmin < wbmax ||
       8 * wbmin > 5 * wbmax)
    {
        dbprintf(2, " [bar0 ratio]\n");
        return(ZBAR_NONE);
    }
    unsigned wbmid = get_width(dcode, (ibar >> 4) & 0xf);
    if(8 * wbmid > 5 * wbmax) {
        dbprintf(2, " [bar2 ratio]\n");
        return(ZBAR_NONE);
    }
    ibar = ((ibar & 0xf) - 1) >> 1;
    dbprintf(2, "(%d)", ibar);

    /* decode combination */
    int ic = ispc * 4 + ibar;
    zassert(ic < 8, ZBAR_NONE, "ic=%d ispc=%d ibar=%d", ic, ispc, ibar);
    int c = codabar_hi[ic];
    codabar->buf[0] = (c & 0x3) | 0x10;

    /* set character direction */
    codabar->direction = c >> 2;

    codabar->element = 4;
    codabar->character = 1;
    codabar->width = codabar->s7;
    dbprintf(1, " start=%c dir=%x [valid start]\n",
             codabar->buf[0] + 0x31, codabar->direction);
    return(ZBAR_PARTIAL);
}

static inline int
codabar_checksum (zbar_decoder_t *dcode,
                  unsigned n)
{
    unsigned chk = 0;
    unsigned char *buf = dcode->buf;
    while(n--)
        chk += *(buf++);
    return(!!(chk & 0xf));
}

static inline zbar_symbol_type_t
codabar_postprocess (zbar_decoder_t *dcode)
{
    codabar_decoder_t *codabar = &dcode->codabar;
    int dir = codabar->direction;
    dcode->direction = 1 - 2 * dir;
    int i, n = codabar->character;
    for(i = 0; i < NIBUF; i++)
        dcode->buf[i] = codabar->buf[i];
    if(dir)
        /* reverse buffer */
        for(i = 0; i < n / 2; i++) {
            unsigned j = n - 1 - i;
            char code = dcode->buf[i];
            dcode->buf[i] = dcode->buf[j];
            dcode->buf[j] = code;
        }

    if(TEST_CFG(codabar->config, ZBAR_CFG_ADD_CHECK)) {
        /* validate checksum */
        if(codabar_checksum(dcode, n))
            return(ZBAR_NONE);
        if(!TEST_CFG(codabar->config, ZBAR_CFG_EMIT_CHECK)) {
            dcode->buf[n - 2] = dcode->buf[n - 1];
            n--;
        }
    }

    for(i = 0; i < n; i++) {
        unsigned c = dcode->buf[i];
        dcode->buf[i] = ((c < 0x14)
                         ? codabar_characters[c]
                         : '?');
    }
    dcode->buflen = i;
    dcode->buf[i] = '\0';
    dcode->modifiers = 0;

    codabar->character = -1;
    return(ZBAR_CODABAR);
}

zbar_symbol_type_t
_zbar_decode_codabar (zbar_decoder_t *dcode)
{
    codabar_decoder_t *codabar = &dcode->codabar;

    /* update latest character width */
    codabar->s7 -= get_width(dcode, 8);
    codabar->s7 += get_width(dcode, 1);

    if(get_color(dcode) != ZBAR_SPACE)
        return(ZBAR_NONE);
    if(codabar->character < 0)
        return(codabar_decode_start(dcode));
    if(codabar->character < 2 &&
       codabar_decode_start(dcode))
        return(ZBAR_PARTIAL);
    if(--codabar->element)
        return(ZBAR_NONE);
    codabar->element = 4;

    dbprintf(1, "      codabar[%c%02d+%x]",
             (codabar->direction) ? '<' : '>',
             codabar->character, codabar->element);

    signed char c = codabar_decode7(dcode);
    dbprintf(1, " %d", c);
    if(c < 0) {
        dbprintf(1, " [aborted]\n");
        goto reset;
    }

    unsigned char *buf;
    if(codabar->character < NIBUF)
        buf = codabar->buf;
    else {
        if(codabar->character >= BUFFER_MIN &&
           size_buf(dcode, codabar->character + 1))
        {
            dbprintf(1, " [overflow]\n");
            goto reset;
        }
        buf = dcode->buf;
    }
    buf[codabar->character++] = c;

    /* lock shared resources */
    if(codabar->character == NIBUF &&
       acquire_lock(dcode, ZBAR_CODABAR))
    {
        codabar->character = -1;
        return(ZBAR_PARTIAL);
    }

    unsigned s = codabar->s7;
    if(c & 0x10) {
        unsigned qz = get_width(dcode, 0);
        if(qz && qz * 2 < s) {
            dbprintf(2, " [invalid qz]\n");
            goto reset;
        }
        unsigned n = codabar->character;
        if(n < CFG(*codabar, ZBAR_CFG_MIN_LEN) ||
           (CFG(*codabar, ZBAR_CFG_MAX_LEN) > 0 &&
            n > CFG(*codabar, ZBAR_CFG_MAX_LEN)))
        {
            dbprintf(2, " [invalid len]\n");
            goto reset;
        }
        if(codabar->character < NIBUF &&
           acquire_lock(dcode, ZBAR_CODABAR))
        {
            codabar->character = -1;
            return(ZBAR_PARTIAL);
        }
        dbprintf(2, " stop=%c", c + 0x31);

        zbar_symbol_type_t sym = codabar_postprocess(dcode);
        if(sym > ZBAR_PARTIAL)
            dbprintf(2, " [valid stop]");
        else {
            release_lock(dcode, ZBAR_CODABAR);
            codabar->character = -1;
        }
        dbprintf(2, "\n");
        return(sym);
    }
    else if(4 * get_width(dcode, 0) > 3 * s) {
        dbprintf(2, " [ics]\n");
        goto reset;
    }

    dbprintf(2, "\n");
    return(ZBAR_NONE);

reset:
    if(codabar->character >= NIBUF)
        release_lock(dcode, ZBAR_CODABAR);
    codabar->character = -1;
    return(ZBAR_NONE);
}
