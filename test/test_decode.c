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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <zebra.h>

zebra_decoder_t *decoder;

static void symbol_handler (zebra_decoder_t *decoder)
{
    zebra_symbol_type_t sym = zebra_decoder_get_type(decoder);
    if(sym <= ZEBRA_PARTIAL)
        return;
    printf("%s%s:%s\n",
           zebra_get_symbol_name(sym),
           zebra_get_addon_name(sym),
           zebra_decoder_get_data(decoder));
    /* FIXME add check! */
}

static void encode_junk (int n)
{
    printf("encode random junk...\n");
    int i;
    for(i = 0; i < n; i++)
        zebra_decode_width(decoder, 10. * (rand() / (RAND_MAX + 1.)));
}

#define FWD 1
#define REV 0

static void encode (unsigned int units,
                    int fwd)
{
    printf(" raw=%x%c\n", units, (fwd) ? '<' : '>');
    if(!fwd)
        while(units && !(units >> 0x1c))
            units <<= 4;

    while(units) {
        unsigned char w = (fwd) ? units & 0xf : units >> 0x1c;
        zebra_decode_width(decoder, w);
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

static void encode_code128b (unsigned char *data)
{
    printf("------------------------------------------------------------\n"
           "encode CODE-128(B): %s\n"
           "    encode START_B: %02x", data, START_B);
    encode(code128[START_B], 0);
    int i, chk = START_B;
    for(i = 0; data[i]; i++) {
        printf("    encode '%c': %02x", data[i], data[i] - 0x20);
        encode(code128[data[i] - 0x20], 0);
        chk += (i + 1) * (data[i] - 0x20);
    }
    chk %= 103;
    printf("    encode checksum: %02x", chk);
    encode(code128[chk], 0);
    printf("    encode STOP: %02x", STOP);
    encode(code128[STOP], 0);
    printf("------------------------------------------------------------\n");
}

static void encode_code128c (unsigned char *data)
{
    printf("------------------------------------------------------------\n"
           "encode CODE-128(C): %s\n"
           "    encode START_C: %02x", data, START_C);
    encode(code128[START_C], 0);
    int i, chk = START_C;
    for(i = 0; data[i]; i += 2) {
        assert(data[i] >= '0');
        assert(data[i + 1] >= '0');
        unsigned char c = (data[i] - '0') * 10 + (data[i + 1] - '0');
        printf("    encode '%c%c': %02d", data[i], data[i + 1], c);
        encode(code128[c], 0);
        chk += (i / 2 + 1) * c;
    }
    chk %= 103;
    printf("    encode checksum: %02x", chk);
    encode(code128[chk], 0);
    printf("    encode STOP: %02x", STOP);
    encode(code128[STOP], 0);
    printf("------------------------------------------------------------\n");
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

static void encode_char39 (unsigned char c)
{
    if(c < 0x20 || c > 0x5a)
        return; /* skip (FIXME) */

    unsigned int raw = code39[c - 0x20];
    if(!raw)
        return; /* skip (FIXME) */

    unsigned int hi = 0;
    int j;
    for(j = 0; j < 8; j++) {
        hi = (hi << 4) | ((raw & 0x100) ? 2 : 1);
        raw <<= 1;
    }
    unsigned int lo = (((raw & 0x100) ? 2 : 1) << 4) | 1;
    printf("    encode '%c': %08x%02x: ", c, hi, lo);
    encode(hi, REV);
    encode(lo, REV);
}

static void encode_code39 (unsigned char *data)
{
    printf("------------------------------------------------------------\n"
           "encode CODE-39: %s\n", data);
    encode(0xa, 0);  /* leading quiet */
    encode_char39('*');
    int i;
    for(i = 0; data[i]; i++)
        encode_char39(data[i]);
    encode_char39('*');
    encode(0xa, 0);  /* trailing quiet */
    printf("------------------------------------------------------------\n");
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

static void calc_ean_parity (unsigned char *data,
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

static void encode_ean13 (unsigned char *data)
{
    int i;
    unsigned char par = ean_parity_encode[data[0] - '0'];

    printf("------------------------------------------------------------\n"
           "encode EAN-13: %s (%02x)\n"
           "    encode start guard:",
           data, par);
    encode(ean_guard[3], FWD);
    for(i = 1; i < 7; i++, par <<= 1) {
        printf("    encode %x%c:", (par >> 5) & 1, data[i]);
        encode(ean_digits[data[i] - '0'], REV ^ ((par >> 5) & 1));
    }
    printf("    encode center guard:");
    encode(ean_guard[5], FWD);
    for(; i < 13; i++) {
        printf("    encode %x%c:", 0, data[i]);
        encode(ean_digits[data[i] - '0'], FWD);
    }
    printf("    encode end guard:");
    encode(ean_guard[3], REV);
    printf("------------------------------------------------------------\n");
}

static void encode_ean8 (unsigned char *data)
{
    int i;
    printf("------------------------------------------------------------\n"
           "encode EAN-8: %s\n"
           "    encode start guard:",
           data);
    encode(ean_guard[3], FWD);
    for(i = 0; i < 4; i++) {
        printf("    encode %c:", data[i]);
        encode(ean_digits[data[i] - '0'], FWD);
    }
    printf("    encode center guard:");
    encode(ean_guard[5], FWD);
    for(; i < 8; i++) {
        printf("    encode %c:", data[i]);
        encode(ean_digits[data[i] - '0'], FWD);
    }
    printf("    encode end guard:");
    encode(ean_guard[3], REV);
    printf("------------------------------------------------------------\n");
}


/*------------------------------------------------------------*/
/* main test flow */

int main (int argc, char **argv)
{
    int i;
    int rnd_size = 9;           /* should be odd */
    srand(0xbabeface);

    /* FIXME TBD:
     *   - random module width (!= 1.0)
     *   - simulate scan speed variance
     *   - simulate dark "swelling" and light "blooming"
     *   - inject parity errors
     */
    decoder = zebra_decoder_create();
    zebra_decoder_set_handler(decoder, symbol_handler);

    encode_junk(rnd_size + 1);

    unsigned char data[32] = { 0 };
    for(i = 0; i < 12; i++)
        data[i] = ((rand() >> 16) % 10) + '0';

    calc_ean_parity(data, 12);
    encode_ean13(data);

    encode_junk(rnd_size);

    data[i] = 0;
    encode_code128c(data);

    encode_junk(rnd_size);

    calc_ean_parity(data, 7);
    encode_ean8(data);

    encode_junk(rnd_size);

    for(i = 0; i < 10; i++)
        data[i] = (rand() >> 16) % 0x5f + 0x20;
    data[i] = 0;

    encode_code128b(data);

    encode_junk(rnd_size);

    /*encode_code39("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ-. $/+%");*/
    encode_code39(data);

    encode_junk(rnd_size);

    zebra_decoder_destroy(decoder);
    return(0);
}
