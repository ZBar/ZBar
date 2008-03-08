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

#include <config.h>
#ifdef DEBUG_DECODER
# include <stdio.h>     /* fprintf */
#endif
#include <stdlib.h>     /* malloc, free */
#include <string.h>     /* memset */
#include <assert.h>

#include <zebra.h>
#include "decoder.h"

#ifdef DEBUG_DECODER
# define dprintf(...) \
    fprintf(stderr, __VA_ARGS__)
#else
# define dprintf(...)
#endif

zebra_decoder_t *zebra_decoder_create ()
{
    zebra_decoder_t *dcode = malloc(sizeof(zebra_decoder_t));
    dcode->buflen = BUFFER_MIN;
    dcode->buf = malloc(dcode->buflen);
    zebra_decoder_reset(dcode);
    return(dcode);
}

void zebra_decoder_destroy (zebra_decoder_t *dcode)
{
    free(dcode);
}

void zebra_decoder_reset (zebra_decoder_t *dcode)
{
    memset(dcode, 0, (long)&dcode->buf - (long)dcode);
#ifdef ENABLE_EAN
    ean_reset(&dcode->ean);
#endif
#ifdef ENABLE_I25
    i25_reset(&dcode->i25);
#endif
#ifdef ENABLE_CODE39
    code39_reset(&dcode->code39);
#endif
#ifdef ENABLE_CODE128
    code128_reset(&dcode->code128);
#endif
}

void zebra_decoder_new_scan (zebra_decoder_t *dcode)
{
    /* soft reset decoder */
    memset(dcode->w, 0, sizeof(dcode->w));
    dcode->lock = 0;
    dcode->idx = 0;
#ifdef ENABLE_EAN
    ean_new_scan(&dcode->ean);
#endif
#ifdef ENABLE_I25
    i25_reset(&dcode->i25);
#endif
#ifdef ENABLE_CODE39
    code39_reset(&dcode->code39);
#endif
#ifdef ENABLE_CODE128
    code128_reset(&dcode->code128);
#endif
}


zebra_color_t zebra_decoder_get_color (const zebra_decoder_t *dcode)
{
    return(get_color(dcode));
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

void *zebra_decoder_get_userdata (const zebra_decoder_t *dcode)
{
    return(dcode->userdata);
}

zebra_symbol_type_t zebra_decoder_get_type (const zebra_decoder_t *dcode)
{
    return(dcode->type);
}

zebra_symbol_type_t zebra_decode_width (zebra_decoder_t *dcode,
                                        unsigned w)
{
    dcode->w[dcode->idx & (DECODE_WINDOW - 1)] = w;
    dprintf("    decode[%x]: w=%d (%g)\n", dcode->idx, w, (w / 32.));

    /* each decoder processes width stream in parallel */
    zebra_symbol_type_t sym = dcode->type = ZEBRA_NONE;

#ifdef ENABLE_EAN
    if((sym = zebra_decode_ean(dcode)))
        dcode->type = sym;
#endif
#ifdef ENABLE_CODE39
    if((sym = zebra_decode_code39(dcode)) > ZEBRA_PARTIAL)
        dcode->type = sym;
#endif
#ifdef ENABLE_CODE128
    if((sym = zebra_decode_code128(dcode)) > ZEBRA_PARTIAL)
        dcode->type = sym;
#endif
#ifdef ENABLE_I25
    if((sym = zebra_decode_i25(dcode)) > ZEBRA_PARTIAL)
        dcode->type = sym;
#endif

    dcode->idx++;
    if(dcode->type) {
        if(dcode->handler)
            dcode->handler(dcode);
        if(dcode->lock && dcode->type > ZEBRA_PARTIAL)
            dcode->lock = 0;
    }
    return(dcode->type);
}
