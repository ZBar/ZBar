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

#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#include <zbar.h>

zbar_decoder_t *decoder;

zbar_symbol_type_t expect_sym;
char *expect_data = NULL;

unsigned seed = 0;
int verbosity = 1;
int rnd_size = 9;  /* NB should be odd */
int iter = 0;      /* test iteration */

#define zprintf(level, format, ...) do {                                \
        if(verbosity >= (level)) {                                      \
            fprintf(stderr, format , ##__VA_ARGS__); \
        }                                                               \
    } while(0)

static inline void print_sep (int level)
{
    zprintf(level,
            "----------------------------------------------------------\n");
}

static void symbol_handler (zbar_decoder_t *decoder)
{
    zbar_symbol_type_t sym = zbar_decoder_get_type(decoder);
    if(sym <= ZBAR_PARTIAL || sym == ZBAR_QRCODE)
        return;
    const char *data = zbar_decoder_get_data(decoder);

    int pass = (sym == expect_sym) && !strcmp(data, expect_data) &&
        zbar_decoder_get_data_length(decoder) == strlen(data);
    pass *= 3;

    zprintf(pass, "decode %s:%s\n", zbar_get_symbol_name(sym), data);

    if(!expect_sym)
        zprintf(0, "UNEXPECTED!\n");
    else
        zprintf(pass, "expect %s:%s\n", zbar_get_symbol_name(expect_sym),
                expect_data);
    if(!pass) {
        zprintf(0, "SEED=%d\n", seed);
        abort();
    }

    expect_sym = ZBAR_NONE;
    free(expect_data);
    expect_data = NULL;
}

static void expect (zbar_symbol_type_t sym,
                    const char *data)
{
    if(expect_sym) {
        zprintf(0, "MISSING %s:%s\n"
                "SEED=%d\n",
                zbar_get_symbol_name(expect_sym), expect_data, seed);
        abort();
    }
    expect_sym = sym;
    expect_data = (data) ? strdup(data) : NULL;
}

static void encode_junk (int n)
{
    if(n > 1)
        zprintf(3, "encode random junk...\n");
    int i;
    for(i = 0; i < n; i++)
        zbar_decode_width(decoder, 20. * (rand() / (RAND_MAX + 1.)) + 1);
}

#define FWD 1
#define REV 0

static void encode (uint64_t units,
                    int fwd)
{
    zprintf(3, " raw=%x%x%c\n", (unsigned)(units >> 32),
            (unsigned)(units & 0xffffffff), (fwd) ? '<' : '>');
    if(!fwd)
        while(units && !(units >> 0x3c))
            units <<= 4;

    while(units) {
        unsigned char w = (fwd) ? units & 0xf : units >> 0x3c;
        zbar_decode_width(decoder, w);
        if(fwd)
            units >>= 4;
        else
            units <<= 4;
    }
}


/*------------------------------------------------------------*/
/* Code 128 encoding */

typedef enum code128_char_e {
    FNC3        = 0x60,
    FNC2        = 0x61,
    SHIFT       = 0x62,
    CODE_C      = 0x63,
    CODE_B      = 0x64,
    CODE_A      = 0x65,
    FNC1        = 0x66,
    START_A     = 0x67,
    START_B     = 0x68,
    START_C     = 0x69,
    STOP        = 0x6a,
} code128_char_t;

static const unsigned int code128[107] = {
    0x212222, 0x222122, 0x222221, 0x121223, /* 00 */
    0x121322, 0x131222, 0x122213, 0x122312,
    0x132212, 0x221213, 0x221312, 0x231212, /* 08 */
    0x112232, 0x122132, 0x122231, 0x113222,
    0x123122, 0x123221, 0x223211, 0x221132, /* 10 */
    0x221231, 0x213212, 0x223112, 0x312131,
    0x311222, 0x321122, 0x321221, 0x312212, /* 18 */
    0x322112, 0x322211, 0x212123, 0x212321,
    0x232121, 0x111323, 0x131123, 0x131321, /* 20 */
    0x112313, 0x132113, 0x132311, 0x211313,
    0x231113, 0x231311, 0x112133, 0x112331, /* 28 */
    0x132131, 0x113123, 0x113321, 0x133121,
    0x313121, 0x211331, 0x231131, 0x213113, /* 30 */
    0x213311, 0x213131, 0x311123, 0x311321,
    0x331121, 0x312113, 0x312311, 0x332111, /* 38 */
    0x314111, 0x221411, 0x431111, 0x111224,
    0x111422, 0x121124, 0x121421, 0x141122, /* 40 */
    0x141221, 0x112214, 0x112412, 0x122114,
    0x122411, 0x142112, 0x142211, 0x241211, /* 48 */
    0x221114, 0x413111, 0x241112, 0x134111,
    0x111242, 0x121142, 0x121241, 0x114212, /* 50 */
    0x124112, 0x124211, 0x411212, 0x421112,
    0x421211, 0x212141, 0x214121, 0x412121, /* 58 */
    0x111143, 0x111341, 0x131141, 0x114113,
    0x114311, 0x411113, 0x411311, 0x113141, /* 60 */
    0x114131, 0x311141, 0x411131,
    0xa211412, 0xa211214, 0xa211232,        /* START_A-START_C (67-69) */
    0x2331112a,                             /* STOP (6a) */
};

static void encode_code128b (char *data)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    print_sep(3);
    zprintf(2, "CODE-128(B): %s\n", data);
    zprintf(3, "    encode START_B: %02x", START_B);
    encode(code128[START_B], 0);
    int i, chk = START_B;
    for(i = 0; data[i]; i++) {
        zprintf(3, "    encode '%c': %02x", data[i], data[i] - 0x20);
        encode(code128[data[i] - 0x20], 0);
        chk += (i + 1) * (data[i] - 0x20);
    }
    chk %= 103;
    zprintf(3, "    encode checksum: %02x", chk);
    encode(code128[chk], 0);
    zprintf(3, "    encode STOP: %02x", STOP);
    encode(code128[STOP], 0);
    print_sep(3);
}

static void encode_code128c (char *data)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    print_sep(3);
    zprintf(2, "CODE-128(C): %s\n", data);
    zprintf(3, "    encode START_C: %02x", START_C);
    encode(code128[START_C], 0);
    int i, chk = START_C;
    for(i = 0; data[i]; i += 2) {
        assert(data[i] >= '0');
        assert(data[i + 1] >= '0');
        unsigned char c = (data[i] - '0') * 10 + (data[i + 1] - '0');
        zprintf(3, "    encode '%c%c': %02d", data[i], data[i + 1], c);
        encode(code128[c], 0);
        chk += (i / 2 + 1) * c;
    }
    chk %= 103;
    zprintf(3, "    encode checksum: %02x", chk);
    encode(code128[chk], 0);
    zprintf(3, "    encode STOP: %02x", STOP);
    encode(code128[STOP], 0);
    print_sep(3);
}

/*------------------------------------------------------------*/
/* Code 93 encoding */

#define CODE93_START_STOP 0x2f

static const unsigned int code93[47 + 1] = {
    0x131112, 0x111213, 0x111312, 0x111411, /* 00 */
    0x121113, 0x121212, 0x121311, 0x111114,
    0x131211, 0x141111, 0x211113, 0x211212, /* 08 */
    0x211311, 0x221112, 0x221211, 0x231111,
    0x112113, 0x112212, 0x112311, 0x122112, /* 10 */
    0x132111, 0x111123, 0x111222, 0x111321,
    0x121122, 0x131121, 0x212112, 0x212211, /* 18 */
    0x211122, 0x211221, 0x221121, 0x222111,
    0x112122, 0x112221, 0x122121, 0x123111, /* 20 */
    0x121131, 0x311112, 0x311211, 0x321111,
    0x112131, 0x113121, 0x211131, 0x121221, /* 28 */
    0x312111, 0x311121, 0x122211,
    0x111141,                               /* START/STOP (2f) */
};

#define S1 0x2b00|
#define S2 0x2c00|
#define S3 0x2d00|
#define S4 0x2e00|

static const unsigned short code93_ext[0x80] = {
    S2'U', S1'A', S1'B', S1'C', S1'D', S1'E', S1'F', S1'G',
    S1'H', S1'I', S1'J', S1'K', S1'L', S1'M', S1'N', S1'O',
    S1'P', S1'Q', S1'R', S1'S', S1'T', S1'U', S1'V', S1'W',
    S1'X', S1'Y', S1'Z', S2'A', S2'B', S2'C', S2'D', S2'E',
    0x26,  S3'A', S3'B', S3'C', 0x27,  0x2a,  S3'F', S3'G',
    S3'H', S3'I', S3'J', 0x29,  S3'L', 0x24,  0x25,  0x28,
    0x00,  0x01,  0x02,  0x03,  0x04,  0x05,  0x06,  0x07,
    0x08,  0x09,  S3'Z', S2'F', S2'G', S2'H', S2'I', S2'J',
    S2'V', 0x0a,  0x0b,  0x0c,  0x0d,  0x0e,  0x0f,  0x10,
    0x11,  0x12,  0x13,  0x14,  0x15,  0x16,  0x17,  0x18,
    0x19,  0x1a,  0x1b,  0x1c,  0x1d,  0x1e,  0x1f,  0x20,
    0x21,  0x22,  0x23,  S2'K', S2'L', S2'M', S2'N', S2'O',
    S2'W', S4'A', S4'B', S4'C', S4'D', S4'E', S4'F', S4'G',
    S4'H', S4'I', S4'J', S4'K', S4'L', S4'M', S4'N', S4'O',
    S4'P', S4'Q', S4'R', S4'S', S4'T', S4'U', S4'V', S4'W',
    S4'X', S4'Y', S4'Z', S2'P', S2'Q', S2'R', S2'S', S2'T',
};

#undef S1
#undef S2
#undef S3
#undef S4

static void encode_char93 (unsigned char c,
                           int dir)
{
    unsigned ext = code93_ext[c];
    unsigned shift = ext >> 8;
    assert(shift < 0x30);
    c = ext & 0xff;
    if(shift) {
        assert(c < 0x80);
        c = code93_ext[c];
    }
    assert(c < 0x30);

    if(shift) {
        encode(code93[(dir) ? shift : c], dir ^ 1);
        encode(code93[(dir) ? c : shift], dir ^ 1);
    }
    else
        encode(code93[c], dir ^ 1);
}

static void encode_code93 (char *data,
                           int dir)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    print_sep(3);

    /* calculate checksums */
    int i, j, chk_c = 0, chk_k = 0, n = 0;
    for(i = 0; data[i]; i++, n++) {
        unsigned c = data[i], ext;
        assert(c < 0x80);
        ext = code93_ext[c];
        n += ext >> 13;
    }

    for(i = 0, j = 0; data[i]; i++, j++) {
        unsigned ext = code93_ext[(unsigned)data[i]];
        unsigned shift = ext >> 8;
        unsigned c = ext & 0xff;
        if(shift) {
            chk_c += shift * (((n - 1 - j) % 20) + 1);
            chk_k += shift * (((n - j) % 15) + 1);
            j++;
            c = code93_ext[c];
        }
        chk_c += c * (((n - 1 - j) % 20) + 1);
        chk_k += c * (((n - j) % 15) + 1);
    }
    chk_c %= 47;
    chk_k += chk_c;
    chk_k %= 47;

    zprintf(2, "CODE-93: %s (n=%x C=%02x K=%02x)\n", data, n, chk_c, chk_k);
    encode(0xa, 0);  /* leading quiet */

    zprintf(3, "    encode %s:", (dir) ? "START" : "STOP");
    if(!dir)
        encode(0x1, REV);
    encode(code93[CODE93_START_STOP], dir ^ 1);
    if(!dir) {
        zprintf(3, "    encode checksum (K): %02x", chk_k);
        encode(code93[chk_k], REV ^ 1);
        zprintf(3, "    encode checksum (C): %02x", chk_c);
        encode(code93[chk_c], REV ^ 1);
    }

    n = strlen(data);
    for(i = 0; i < n; i++) {
        unsigned char c = data[(dir) ? i : (n - i - 1)];
        zprintf(3, "    encode '%c':", c);
        encode_char93(c, dir);
    }

    if(dir) {
        zprintf(3, "    encode checksum (C): %02x", chk_c);
        encode(code93[chk_c], FWD ^ 1);
        zprintf(3, "    encode checksum (K): %02x", chk_k);
        encode(code93[chk_k], FWD ^ 1);
    }
    zprintf(3, "    encode %s:", (dir) ? "STOP" : "START");
    encode(code93[CODE93_START_STOP], dir ^ 1);
    if(dir)
        encode(0x1, FWD);

    encode(0xa, 0);  /* trailing quiet */
    print_sep(3);
}

/*------------------------------------------------------------*/
/* Code 39 encoding */

static const unsigned int code39[91-32] = {
    0x0c4, 0x000, 0x000, 0x000,  0x0a8, 0x02a, 0x000, 0x000, /* 20 */
    0x000, 0x000, 0x094, 0x08a,  0x000, 0x085, 0x184, 0x0a2, /* 28 */
    0x034, 0x121, 0x061, 0x160,  0x031, 0x130, 0x070, 0x025, /* 30 */
    0x124, 0x064, 0x000, 0x000,  0x000, 0x000, 0x000, 0x000, /* 38 */
    0x000, 0x109, 0x049, 0x148,  0x019, 0x118, 0x058, 0x00d, /* 40 */
    0x10c, 0x04c, 0x01c, 0x103,  0x043, 0x142, 0x013, 0x112, /* 48 */
    0x052, 0x007, 0x106, 0x046,  0x016, 0x181, 0x0c1, 0x1c0, /* 50 */
    0x091, 0x190, 0x0d0,                                     /* 58 */
};

/* FIXME configurable/randomized ratio, ics */
/* FIXME check digit option, ASCII escapes */

static void convert_code39 (char *data)
{
    char *src, *dst;
    for(src = data, dst = data; *src; src++) {
        char c = *src;
        if(c >= 'a' && c <= 'z')
            *(dst++) = c - ('a' - 'A');
        else if(c == ' ' ||
                c == '$' || c == '%' ||
                c == '+' || c == '-' ||
                (c >= '.' && c <= '9') ||
                (c >= 'A' && c <= 'Z'))
            *(dst++) = c;
        else
            /* skip (FIXME) */;
    }
    *dst = 0;
}

static void encode_char39 (unsigned char c,
                           unsigned ics)
{
    assert(0x20 <= c && c <= 0x5a);
    unsigned int raw = code39[c - 0x20];
    if(!raw)
        return; /* skip (FIXME) */

    uint64_t enc = 0;
    int j;
    for(j = 0; j < 9; j++) {
        enc = (enc << 4) | ((raw & 0x100) ? 2 : 1);
        raw <<= 1;
    }
    enc = (enc << 4) | ics;
    zprintf(3, "    encode '%c': %02x%08x: ", c,
            (unsigned)(enc >> 32), (unsigned)(enc & 0xffffffff));
    encode(enc, REV);
}

static void encode_code39 (char *data)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    print_sep(3);
    zprintf(2, "CODE-39: %s\n", data);
    encode(0xa, 0);  /* leading quiet */
    encode_char39('*', 1);
    int i;
    for(i = 0; data[i]; i++)
        if(data[i] != '*') /* skip (FIXME) */
            encode_char39(data[i], 1);
    encode_char39('*', 0xa);  /* w/trailing quiet */
    print_sep(3);
}

#if 0
/*------------------------------------------------------------*/
/* PDF417 encoding */

/* hardcoded test message: "hello world" */
#define PDF417_ROWS 3
#define PDF417_COLS 3
static const unsigned pdf417_msg[PDF417_ROWS][PDF417_COLS] = {
    { 007, 817, 131 },
    { 344, 802, 437 },
    { 333, 739, 194 },
};

#define PDF417_START UINT64_C(0x81111113)
#define PDF417_STOP  UINT64_C(0x711311121)
#include "pdf417_encode.h"

static int calc_ind417 (int mod,
                        int r,
                        int cols)
{
    mod = (mod + 3) % 3;
    int cw = 30 * (r / 3);
    if(!mod)
        return(cw + cols - 1);
    else if(mod == 1)
        return(cw + (PDF417_ROWS - 1) % 3);
    assert(mod == 2);
    return(cw + (PDF417_ROWS - 1) / 3);
}

static void encode_row417 (int r,
                           const unsigned *cws,
                           int cols,
                           int dir)
{
    int k = r % 3;

    zprintf(3, "    [%d] encode %s:", r, (dir) ? "stop" : "start");
    encode((dir) ? PDF417_STOP : PDF417_START, dir);

    int cw = calc_ind417(k + !dir, r, cols);
    zprintf(3, "    [%d,%c] encode %03d(%d): ", r, (dir) ? 'R' : 'L', cw, k);
    encode(pdf417_encode[cw][k], dir);

    int c;
    for(c = 0; c < cols; c++) {
        cw = cws[c];
        zprintf(3, "    [%d,%d] encode %03d(%d): ", r, c, cw, k);
        encode(pdf417_encode[cw][k], dir);
    }

    cw = calc_ind417(k + dir, r, cols);
    zprintf(3, "    [%d,%c] encode %03d(%d): ", r, (dir) ? 'L' : 'R', cw, k);
    encode(pdf417_encode[cw][k], dir);

    zprintf(3, "    [%d] encode %s:", r, (dir) ? "start" : "stop");
    encode((dir) ? PDF417_START : PDF417_STOP, dir);
}

static void encode_pdf417 (char *data)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    print_sep(3);
    zprintf(2, "PDF417: hello world\n");
    encode(0xa, 0);

    int r;
    for(r = 0; r < PDF417_ROWS; r++) {
        encode_row417(r, pdf417_msg[r], PDF417_COLS, r & 1);
        encode(0xa, 0);
    }

    print_sep(3);
}
#endif

/*------------------------------------------------------------*/
/* Codabar encoding */

static const unsigned int codabar[20] = {
    0x03, 0x06, 0x09, 0x60, 0x12, 0x42, 0x21, 0x24,
    0x30, 0x48, 0x0c, 0x18, 0x45, 0x51, 0x54, 0x15,
    0x1a, 0x29, 0x0b, 0x0e,
};

static const char codabar_char[0x14] =
    "0123456789-$:/.+ABCD";

/* FIXME configurable/randomized ratio, ics */
/* FIXME check digit option */

static char *convert_codabar (char *src)
{
    unsigned len = strlen(src);
    char tmp[4] = { 0, };
    if(len < 2) {
        unsigned delim = rand() >> 8;
        tmp[0] = delim & 3;
        if(len)
            tmp[1] = src[0];
        tmp[len + 1] = (delim >> 2) & 3;
        len += 2;
        src = tmp;
    }

    char *result = malloc(len + 1);
    char *dst = result;
    *(dst++) = ((*(src++) - 1) & 0x3) + 'A';
    for(len--; len > 1; len--) {
        char c = *(src++);
        if(c >= '0' && c <= '9')
            *(dst++) = c;
        else if(c == '-' || c == '$' || c == ':' || c == '/' ||
                c == '.' || c == '+')
            *(dst++) = c;
        else
            *(dst++) = codabar_char[c % 0x10];
    }
    *(dst++) = ((*(src++) - 1) & 0x3) + 'A';
    *dst = 0;
    return(result);
}

static void encode_codachar (unsigned char c,
                             unsigned ics,
                             int dir)
{
    unsigned int idx;
    if(c >= '0' && c <= '9')
        idx = c - '0';
    else if(c >= 'A' && c <= 'D')
        idx = c - 'A' + 0x10;
    else
        switch(c)
        {
        case '-': idx = 0xa; break;
        case '$': idx = 0xb; break;
        case ':': idx = 0xc; break;
        case '/': idx = 0xd; break;
        case '.': idx = 0xe; break;
        case '+': idx = 0xf; break;
        default:
            assert(0);
        }

    assert(idx < 0x14);
    unsigned int raw = codabar[idx];

    uint32_t enc = 0;
    int j;
    for(j = 0; j < 7; j++, raw <<= 1)
        enc = (enc << 4) | ((raw & 0x40) ? 3 : 1);
    zprintf(3, "    encode '%c': %07x: ", c, enc);
    if(dir)
        enc = (enc << 4) | ics;
    else
        enc |= ics << 28;
    encode(enc, 1 - dir);
}

static void encode_codabar (char *data,
                            int dir)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    print_sep(3);
    zprintf(2, "CODABAR: %s\n", data);
    encode(0xa, 0);  /* leading quiet */
    int i, n = strlen(data);
    for(i = 0; i < n; i++) {
        int j = (dir) ? i : n - i - 1;
        encode_codachar(data[j], (i < n - 1) ? 1 : 0xa, dir);
    }
    print_sep(3);
}

/*------------------------------------------------------------*/
/* Interleaved 2 of 5 encoding */

static const unsigned char i25[10] = {
    0x06, 0x11, 0x09, 0x18, 0x05, 0x14, 0x0c, 0x03, 0x12, 0x0a,
};

static void encode_i25 (char *data,
                        int dir)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    print_sep(3);
    zprintf(2, "Interleaved 2 of 5: %s\n", data);
    zprintf(3, "    encode start:");
    encode((dir) ? 0xa1111 : 0xa112, 0);

    /* FIXME rev case data reversal */
    int i;
    for(i = (strlen(data) & 1) ? -1 : 0; i < 0 || data[i]; i += 2) {
        /* encode 2 digits */
        unsigned char c0 = (i < 0) ? 0 : data[i] - '0';
        unsigned char c1 = data[i + 1] - '0';
        zprintf(3, "    encode '%d%d':", c0, c1);
        assert(c0 < 10);
        assert(c1 < 10);

        c0 = i25[c0];
        c1 = i25[c1];

        /* interleave */
        uint64_t enc = 0;
        int j;
        for(j = 0; j < 5; j++) {
            enc <<= 8;
            enc |= (c0 & 1) ? 0x02 : 0x01;
            enc |= (c1 & 1) ? 0x20 : 0x10;
            c0 >>= 1;
            c1 >>= 1;
        }
        encode(enc, dir);
    }

    zprintf(3, "    encode end:");
    encode((dir) ? 0x211a : 0x1111a, 0);
    print_sep(3);
}

/*------------------------------------------------------------*/
/* DataBar encoding */


/* character encoder reference algorithm from ISO/IEC 24724:2009 */

struct rss_group {
    int T_odd, T_even, n_odd, w_max;
};

static const struct rss_group databar_groups_outside[] = {
    { 161,   1, 12, 8 },
    {  80,  10, 10, 6 },
    {  31,  34,  8, 4 },
    {  10,  70,  6, 3 },
    {   1, 126,  4, 1 },
    {   0, }
};

static const struct rss_group databar_groups_inside[] = {
    {  4, 84,  5, 2 },
    { 20, 35,  7, 4 },
    { 48, 10,  9, 6 },
    { 81,  1, 11, 8 },
    {  0, }
};

static const uint32_t databar_finders[9] = {
    0x38211, 0x35511, 0x33711, 0x31911, 0x27411,
    0x25611, 0x23811, 0x15711, 0x13911,
};

int combins (int n,
             int r)
{
    int i, j;
    int maxDenom, minDenom;
    int val;
    if(n-r > r) {
        minDenom = r;
        maxDenom = n-r;
    }
    else {
        minDenom = n-r;
        maxDenom = r;
    }
    val = 1;
    j = 1;
    for(i = n; i > maxDenom; i--) {
        val *= i;
        if(j <= minDenom) {
            val /= j;
            j++;
        }
    }
    for(; j <= minDenom; j++)
        val /= j;
    return(val);
}

void getRSSWidths (int val,
                   int n,
                   int elements,
                   int maxWidth,
                   int noNarrow,
                   int *widths)
{
    int narrowMask = 0;
    int bar;
    for(bar = 0; bar < elements - 1; bar++) {
        int elmWidth, subVal;
        for(elmWidth = 1, narrowMask |= (1<<bar);
            ;
            elmWidth++, narrowMask &= ~(1<<bar))
        {
            subVal = combins(n-elmWidth-1, elements-bar-2);
            if((!noNarrow) && !narrowMask &&
                (n-elmWidth-(elements-bar-1) >= elements-bar-1))
                subVal -= combins(n-elmWidth-(elements-bar), elements-bar-2);
            if(elements-bar-1 > 1) {
                int mxwElement, lessVal = 0;
                for (mxwElement = n-elmWidth-(elements-bar-2);
                     mxwElement > maxWidth;
                     mxwElement--)
                    lessVal += combins(n-elmWidth-mxwElement-1, elements-bar-3);
                subVal -= lessVal * (elements-1-bar);
            }
            else if (n-elmWidth > maxWidth)
                subVal--;
            val -= subVal;
            if(val < 0)
                break;
        }
        val += subVal;
        n -= elmWidth;
        widths[bar] = elmWidth;
    }
    widths[bar] = n;
}

static uint64_t encode_databar_char (unsigned val,
                                     const struct rss_group *grp,
                                     int nmodules,
                                     int nelems,
                                     int dir)
{
    int G_sum = 0;
    while(1) {
        assert(grp->T_odd);
        int sum = G_sum + grp->T_odd * grp->T_even;
        if(val >= sum)
            G_sum = sum;
        else
            break;
        grp++;
    }

    zprintf(3, "char=%d", val);

    int V_grp = val - G_sum;
    int V_odd, V_even;
    if(!dir) {
        V_odd = V_grp / grp->T_even;
        V_even = V_grp % grp->T_even;
    }
    else {
        V_even = V_grp / grp->T_odd;
        V_odd = V_grp % grp->T_odd;
    }

    zprintf(3, " G_sum=%d T_odd=%d T_even=%d n_odd=%d w_max=%d V_grp=%d\n",
            G_sum, grp->T_odd, grp->T_even, grp->n_odd, grp->w_max, V_grp);

    int odd[16];
    getRSSWidths(V_odd, grp->n_odd, nelems, grp->w_max, !dir, odd);
    zprintf(3, "    V_odd=%d odd=%d%d%d%d",
            V_odd, odd[0], odd[1], odd[2], odd[3]);

    int even[16];
    getRSSWidths(V_even, nmodules - grp->n_odd, nelems, 9 - grp->w_max,
                 dir, even);
    zprintf(3, " V_even=%d even=%d%d%d%d",
            V_even, even[0], even[1], even[2], even[3]);

    uint64_t units = 0;
    int i;
    for(i = 0; i < nelems; i++)
        units = (units << 8) | (odd[i] << 4) | even[i];

    zprintf(3, " raw=%"PRIx64"\n", units);
    return(units);
}

#define SWAP(a, b) do { \
        uint32_t tmp = (a); \
        (a) = (b); \
        (b) = tmp; \
    } while(0);

static void encode_databar (char *data,
                            int dir)
{
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);

    print_sep(3);
    zprintf(2, "DataBar: %s\n", data);

    uint32_t v[4] = { 0, };
    int i, j;
    for(i = 0; i < 14; i++) {
        for(j = 0; j < 4; j++)
            v[j] *= 10;
        assert(data[i]);
        v[0] += data[i] - '0';
        v[1] += v[0] / 1597;
        v[0] %= 1597;
        v[2] += v[1] / 2841;
        v[1] %= 2841;
        v[3] += v[2] / 1597;
        v[2] %= 1597;
        /*printf("    [%d] %c (%d,%d,%d,%d)\n",
               i, data[i], v[0], v[1], v[2], v[3]);*/
    }
    zprintf(3, "chars=(%d,%d,%d,%d)\n", v[3], v[2], v[1], v[0]);

    uint32_t c[4] = {
        encode_databar_char(v[3], databar_groups_outside, 16, 4, 0),
        encode_databar_char(v[2], databar_groups_inside, 15, 4, 1),
        encode_databar_char(v[1], databar_groups_outside, 16, 4, 0),
        encode_databar_char(v[0], databar_groups_inside, 15, 4, 1),
    };

    int chk = 0, w = 1;
    for(i = 0; i < 4; i++, chk %= 79, w %= 79)
        for(j = 0; j < 8; j++, w *= 3)
            chk += ((c[i] >> (28 - j * 4)) & 0xf) * w;
    zprintf(3, "chk=%d\n", chk);

    if(chk >= 8) chk++;
    if(chk >= 72) chk++;
    int C_left = chk / 9;
    int C_right = chk % 9;

    if(dir == REV) {
        SWAP(C_left, C_right);
        SWAP(c[0], c[2]);
        SWAP(c[1], c[3]);
        SWAP(v[0], v[2]);
        SWAP(v[1], v[3]);
    }

    zprintf(3, "    encode start guard:");
    encode_junk(dir);
    encode(0x1, FWD);

    zprintf(3, "encode char[0]=%d", v[3]);
    encode(c[0], REV);

    zprintf(3, "encode left finder=%d", C_left);
    encode(databar_finders[C_left], REV);

    zprintf(3, "encode char[1]=%d", v[2]);
    encode(c[1], FWD);

    zprintf(3, "encode char[3]=%d", v[0]);
    encode(c[3], REV);

    zprintf(3, "encode right finder=%d", C_right);
    encode(databar_finders[C_right], FWD);

    zprintf(3, "encode char[2]=%d", v[1]);
    encode(c[2], FWD);

    zprintf(3, "    encode end guard:");
    encode(0x1, FWD);
    encode_junk(!dir);
    print_sep(3);
}


/*------------------------------------------------------------*/
/* EAN/UPC encoding */

static const unsigned int ean_digits[10] = {
    0x1123, 0x1222, 0x2212, 0x1141, 0x2311,
    0x1321, 0x4111, 0x2131, 0x3121, 0x2113,
};

static const unsigned int ean_guard[] = {
    0, 0,
    0x11,       /* [2] add-on delineator */
    0x1117,     /* [3] normal guard bars */
    0x2117,     /* [4] add-on guard bars */
    0x11111,    /* [5] center guard bars */
    0x111111    /* [6] "special" guard bars */
};

static const unsigned char ean_parity_encode[] = {
    0x3f,       /* AAAAAA = 0 */
    0x34,       /* AABABB = 1 */
    0x32,       /* AABBAB = 2 */
    0x31,       /* AABBBA = 3 */
    0x2c,       /* ABAABB = 4 */
    0x26,       /* ABBAAB = 5 */
    0x23,       /* ABBBAA = 6 */
    0x2a,       /* ABABAB = 7 */
    0x29,       /* ABABBA = 8 */
    0x25,       /* ABBABA = 9 */
};

static const unsigned char addon_parity_encode[] = {
    0x07,       /* BBAAA = 0 */
    0x0b,       /* BABAA = 1 */
    0x0d,       /* BAABA = 2 */
    0x0e,       /* BAAAB = 3 */
    0x13,       /* ABBAA = 4 */
    0x19,       /* AABBA = 5 */
    0x1c,       /* AAABB = 6 */
    0x15,       /* ABABA = 7 */
    0x16,       /* ABAAB = 8 */
    0x1a,       /* AABAB = 9 */
};

static void calc_ean_parity (char *data,
                             int n)
{
    int i, chk = 0;
    for(i = 0; i < n; i++) {
        unsigned char c = data[i] - '0';
        chk += ((i ^ n) & 1) ? c * 3 : c;
    }
    chk %= 10;
    if(chk)
        chk = 10 - chk;
    data[i++] = '0' + chk;
    data[i] = 0;
}

static void encode_ean13 (char *data)
{
    int i;
    unsigned char par = ean_parity_encode[data[0] - '0'];
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);

    print_sep(3);
    zprintf(2, "EAN-13: %s (%02x)\n", data, par);
    zprintf(3, "    encode start guard:");
    encode(ean_guard[3], FWD);
    for(i = 1; i < 7; i++, par <<= 1) {
        zprintf(3, "    encode %x%c:", (par >> 5) & 1, data[i]);
        encode(ean_digits[data[i] - '0'], (par >> 5) & 1);
    }
    zprintf(3, "    encode center guard:");
    encode(ean_guard[5], FWD);
    for(; i < 13; i++) {
        zprintf(3, "    encode %x%c:", 0, data[i]);
        encode(ean_digits[data[i] - '0'], FWD);
    }
    zprintf(3, "    encode end guard:");
    encode(ean_guard[3], REV);
    print_sep(3);
}

static void encode_ean8 (char *data)
{
    int i;
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    print_sep(3);
    zprintf(2, "EAN-8: %s\n", data);
    zprintf(3, "    encode start guard:");
    encode(ean_guard[3], FWD);
    for(i = 0; i < 4; i++) {
        zprintf(3, "    encode %c:", data[i]);
        encode(ean_digits[data[i] - '0'], FWD);
    }
    zprintf(3, "    encode center guard:");
    encode(ean_guard[5], FWD);
    for(; i < 8; i++) {
        zprintf(3, "    encode %c:", data[i]);
        encode(ean_digits[data[i] - '0'], FWD);
    }
    zprintf(3, "    encode end guard:");
    encode(ean_guard[3], REV);
    print_sep(3);
}

static void encode_addon (char *data,
                          unsigned par,
                          int n)
{
    int i;
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);

    print_sep(3);
    zprintf(2, "EAN-%d: %s (par=%02x)\n", n, data, par);
    zprintf(3, "    encode start guard:");
    encode(ean_guard[4], FWD);
    for(i = 0; i < n; i++, par <<= 1) {
        zprintf(3, "    encode %x%c:", (par >> (n - 1)) & 1, data[i]);
        encode(ean_digits[data[i] - '0'], (par >> (n - 1)) & 1);
        if(i < n - 1) {
	    zprintf(3, "    encode delineator:");
            encode(ean_guard[2], FWD);
        }
    }
    zprintf(3, "    encode trailing qz:");
    encode(0x7, FWD);
    print_sep(3);
}

static void encode_ean5 (char *data)
{
    unsigned chk = ((data[0] - '0' + data[2] - '0' + data[4] - '0') * 3 +
                    (data[1] - '0' + data[3] - '0') * 9) % 10;
    encode_addon(data, addon_parity_encode[chk], 5);
}

static void encode_ean2 (char *data)
{
    unsigned par = (~(10 * (data[0] - '0') + data[1] - '0')) & 3;
    encode_addon(data, par, 2);
}


/*------------------------------------------------------------*/
/* main test flow */

int test_databar_F_1 ()
{
    expect(ZBAR_DATABAR, "0124012345678905");
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    encode(0x11, 0);
    encode(0x31111333, 0);
    encode(0x13911, 0);
    encode(0x31131231, 0);
    encode(0x11214222, 0);
    encode(0x11553, 0);
    encode(0x21231313, 0);
    encode(0x1, 0);
    encode_junk(rnd_size);
    return(0);
}

int test_databar_F_3 ()
{
    expect(ZBAR_DATABAR_EXP, "1012A");
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    encode(0x11, 0);
    encode(0x11521151, 0);
    encode(0x18411, 0);
    encode(0x13171121, 0);
    encode(0x11521232, 0);
    encode(0x11481, 0);
    encode(0x23171111, 0);
    encode(0x1, 0);
    encode_junk(rnd_size);
    return(0);
}

int test_orange ()
{
    char data[32] = "0100845963000052";
    expect(ZBAR_DATABAR, data);
    assert(zbar_decoder_get_color(decoder) == ZBAR_SPACE);
    encode(0x1, 0);
    encode(0x23212321, 0);   // data[0]
    encode(0x31911, 0);      // finder[?] = 3
    encode(0x21121215, 1);   // data[1]
    encode(0x41111133, 0);   // data[3]
    encode(0x23811, 1);      // finder[?] = 6
    encode(0x11215141, 1);   // data[2]
    encode(0x11, 0);
    encode_junk(rnd_size);

    expect(ZBAR_DATABAR, data);
    data[1] = '0';
    encode_databar(data + 1, FWD);
    encode_junk(rnd_size);
    return(0);
}

int test_numeric (char *data)
{
    char tmp[32] = "01";
    strncpy(tmp + 2, data + 1, 13);
    calc_ean_parity(tmp + 2, 13);
    expect(ZBAR_DATABAR, tmp);

    tmp[1] = data[0] & '1';
    encode_databar(tmp + 1, (rand() >> 8) & 1);

    encode_junk(rnd_size);

    data[strlen(data) & ~1] = 0;
    expect(ZBAR_CODE128, data);
    encode_code128c(data);

    encode_junk(rnd_size);

    expect(ZBAR_I25, data);
    encode_i25(data, FWD);

    encode_junk(rnd_size);
#if 0 /* FIXME encoding broken */
    encode_i25(data, REV);

    encode_junk(rnd_size);
#endif

    char *cdb = convert_codabar(data);
    expect(ZBAR_CODABAR, cdb);
    encode_codabar(cdb, FWD);
    encode_junk(rnd_size);

    expect(ZBAR_CODABAR, cdb);
    encode_codabar(cdb, REV);
    encode_junk(rnd_size);
    free(cdb);

    calc_ean_parity(data + 2, 12);
    expect(ZBAR_EAN13, data + 2);
    encode_ean13(data + 2);
    encode_junk(rnd_size);

    calc_ean_parity(data + 7, 7);
    expect(ZBAR_EAN8, data + 7);
    encode_ean8(data + 7);

    encode_junk(rnd_size);

    data[5] = 0;
    expect(ZBAR_EAN5, data);
    encode_ean5(data);

    encode_junk(rnd_size);

    data[2] = 0;
    expect(ZBAR_EAN2, data);
    encode_ean2(data);
    encode_junk(rnd_size);

    expect(ZBAR_NONE, NULL);
    return(0);
}

int test_alpha (char *data)
{
    expect(ZBAR_CODE128, data);
    encode_code128b(data);

    encode_junk(rnd_size);

    expect(ZBAR_CODE93, data);
    encode_code93(data, FWD);

    encode_junk(rnd_size);

    expect(ZBAR_CODE93, data);
    encode_code93(data, REV);

    encode_junk(rnd_size);

    char *cdb = convert_codabar(data);
    expect(ZBAR_CODABAR, cdb);
    encode_codabar(cdb, FWD);
    encode_junk(rnd_size);

    expect(ZBAR_CODABAR, cdb);
    encode_codabar(cdb, REV);
    encode_junk(rnd_size);
    free(cdb);

    convert_code39(data);
    expect(ZBAR_CODE39, data);
    encode_code39(data);

    encode_junk(rnd_size);

#if 0 /* FIXME decoder unfinished */
    encode_pdf417(data);

    encode_junk(rnd_size);
#endif

    expect(ZBAR_NONE, NULL);
    return(0);
}

int test1 ()
{
    print_sep(2);
    if(!seed)
        seed = 0xbabeface;
    zprintf(1, "[%d] SEED=%d\n", iter++, seed);
    srand(seed);
    if(/* EAN-2 within DataBar (020596539169270)
        * (FIXME require COMPOSITE for addons)
        */
       seed == -862734747)
    {
        zprintf(2, "    FIXME known failure\n");
        return(2);
    }

    int i;
    char data[32] = { 0, };
    for(i = 0; i < 14; i++)
        data[i] = (rand() % 10) + '0';

    test_numeric(data);

    for(i = 0; i < 10; i++)
        data[i] = (rand() % 0x5f) + 0x20;
    data[i] = 0;

    test_alpha(data);
    return(0);
}

/* FIXME TBD:
 *   - random module width (!= 1.0)
 *   - simulate scan speed variance
 *   - simulate dark "swelling" and light "blooming"
 *   - inject parity errors
 */

int main (int argc, char **argv)
{
    int n, i, j;
    char *end;

    decoder = zbar_decoder_create();
    /* allow empty CODE39 symbologies */
    zbar_decoder_set_config(decoder, ZBAR_CODE39, ZBAR_CFG_MIN_LEN, 0);
    /* enable addons */
    zbar_decoder_set_config(decoder, ZBAR_EAN2, ZBAR_CFG_ENABLE, 1);
    zbar_decoder_set_config(decoder, ZBAR_EAN5, ZBAR_CFG_ENABLE, 1);
    zbar_decoder_set_handler(decoder, symbol_handler);

    encode_junk(rnd_size + 1);

    for(i = 1; i < argc; i++) {
        if(argv[i][0] != '-') {
            fprintf(stderr, "ERROR: unknown argument: %s\n", argv[i]);
            return(2);
        }
        for(j = 1; argv[i][j]; j++) {
            switch(argv[i][j])
            {
            case 'q': verbosity = 0; break;
            case 'v': verbosity++; break;
            case 'r':
                seed = time(NULL);
                srand(seed);
                seed = (rand() << 8) ^ rand();
                zprintf(0, "-r SEED=%d\n", seed);
                break;

            case 's':
                if(!argv[i][++j] && !(j = 0) && ++i >= argc) {
                    fprintf(stderr, "ERROR: -s needs <seed> argument\n");
                    return(2);
                }
                long s = strtol(argv[i] + j, &end, 0);
                seed = s;
                if((!isdigit(argv[i][j]) && argv[i][j] != '-') ||
                   !s || s == LONG_MAX || s == LONG_MIN) {
                    fprintf(stderr, "ERROR: invalid <seed>: \"%s\"\n",
                            argv[i] + j);
                    return(2);
                }
                j = end - argv[i] - 1;
                break;

            case 'n':
                if(!argv[i][++j] && !(j = 0) && ++i >= argc) {
                    fprintf(stderr, "ERROR: -n needs <num> argument\n");
                    return(2);
                }
                n = strtol(argv[i] + j, &end, 0);
                if(!isdigit(argv[i][j]) || !n) {
                    fprintf(stderr, "ERROR: invalid <num>: \"%s\"\n",
                            argv[i] + j);
                    return(2);
                }
                j = end - argv[i] - 1;

                while(n--) {
                    test1();
                    seed = (rand() << 8) ^ rand();
                }
                break;
            }
        }
    }

    if(!iter) {
        test_databar_F_1();
        test_databar_F_3();
        test_orange();
        test1();
    }

    /* FIXME "Ran %d iterations in %gs\n\nOK\n" */

    zbar_decoder_destroy(decoder);
    return(0);
}
