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

#include <stdlib.h>
#include <stdio.h>

#include <zebra.h>

static const unsigned int digits[10] = {
    0x1123, 0x1222, 0x2212, 0x1141, 0x2311,
    0x1321, 0x4111, 0x2131, 0x3121, 0x2113,
};

static const unsigned int guard[] = {
    0, 0,
    0x11,       /* [2] add-on delineator */
    0x1117,     /* [3] normal guard bars */
    0x2117,     /* [4] add-on guard bars */
    0x11111,    /* [5] center guard bars */
    0x111111    /* [6] "special" guard bars */
};

static const unsigned char parity_encode[] = {
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

zebra_decoder_t *decoder;

static void encode_junk (int n)
{
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

static zebra_symbol_type_t encode_ean13 (unsigned char *data)
{
    int i;
    unsigned char par = parity_encode[data[0]];

    printf("------------------------------------------------------------\n"
           "encode EAN-13: ");
    for(i = 0; i < 13; i++)
        printf("%d", data[i]);
    printf("(%02x)\n"
           "    encode start guard:", par);
    encode(guard[3], FWD);
    for(i = 1; i < 7; i++, par <<= 1) {
        printf("    encode %x%d:", (par >> 5) & 1, data[i]);
        encode(digits[data[i]], FWD ^ ((par >> 5) & 1));
    }
    printf("    encode center guard:");
    encode(guard[5], FWD);
    for(; i < 13; i++) {
        printf("    encode %x%d:", 0, data[i]);
        encode(digits[data[i]], FWD);
    }
    printf("    encode end guard:");
    encode(guard[3], REV);

    return(0);
}

int main (int argc, char **argv)
{
    int i;
    int rnd_size = 10; /* should be even */
    srand(0xbabeface);

    /* FIXME TBD:
     *   - random module width (!= 1.0)
     *   - simulate scan speed variance
     *   - simulate dark "swelling" and light "blooming"
     *   - inject parity errors
     */
    decoder = zebra_decoder_create();

    encode_junk(rnd_size);

    unsigned char data[14];
    unsigned int chk = 0;
    for(i = 0; i < 12; i++) {
        data[i] = (rand() >> 16) % 10;
        chk += (i & 1) ? data[i] * 3 : data[i];
    }
    data[12] = chk % 10;
    data[13] = 0;

    encode_ean13(data);

    encode_junk(rnd_size);

    zebra_decoder_destroy(decoder);
    return(0);
}
