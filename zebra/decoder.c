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
#include <stdlib.h>     /* malloc, free */
#include <stdio.h>      /* snprintf */
#include <string.h>     /* memset, strlen */

#include <zebra.h>
#include "decoder.h"

#if defined(DEBUG_DECODER) || defined(DEBUG_EAN) || defined(DEBUG_CODE128) || \
    defined(DEBUG_CODE39) || defined(DEBUG_I25)
# define DEBUG_LEVEL 1
#endif
#include "debug.h"

zebra_decoder_t *zebra_decoder_create ()
{
    zebra_decoder_t *dcode = malloc(sizeof(zebra_decoder_t));
    dcode->buflen = BUFFER_MIN;
    dcode->buf = malloc(dcode->buflen);
    dcode->handler = dcode->userdata = NULL;

    /* initialize default configs */
#ifdef ENABLE_EAN
    dcode->ean.enable = 1;
    dcode->ean.ean13_config = ((1 << ZEBRA_CFG_ENABLE) |
                               (1 << ZEBRA_CFG_EMIT_CHECK));
    dcode->ean.ean8_config = ((1 << ZEBRA_CFG_ENABLE) |
                              (1 << ZEBRA_CFG_EMIT_CHECK));
    dcode->ean.upca_config = 1 << ZEBRA_CFG_EMIT_CHECK;
    dcode->ean.upce_config = 1 << ZEBRA_CFG_EMIT_CHECK;
    dcode->ean.isbn10_config = 1 << ZEBRA_CFG_EMIT_CHECK;
    dcode->ean.isbn13_config = 1 << ZEBRA_CFG_EMIT_CHECK;
#endif
#ifdef ENABLE_I25
    dcode->i25.config = 1 << ZEBRA_CFG_ENABLE;
#endif
#ifdef ENABLE_CODE39
    dcode->code39.config = 1 << ZEBRA_CFG_ENABLE;
#endif
#ifdef ENABLE_CODE128
    dcode->code128.config = 1 << ZEBRA_CFG_ENABLE;
#endif
#ifdef ENABLE_PDF417
    dcode->pdf417.config = 1 << ZEBRA_CFG_ENABLE;
#endif

    zebra_decoder_reset(dcode);
    return(dcode);
}

void zebra_decoder_destroy (zebra_decoder_t *dcode)
{
    if(dcode->buf)
        free(dcode->buf);
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
#ifdef ENABLE_PDF417
    pdf417_reset(&dcode->pdf417);
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
#ifdef ENABLE_PDF417
    pdf417_reset(&dcode->pdf417);
#endif
}


zebra_color_t zebra_decoder_get_color (const zebra_decoder_t *dcode)
{
    return(get_color(dcode));
}

const char *zebra_decoder_get_data (const zebra_decoder_t *dcode)
{
    return((char*)dcode->buf);
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
    dprintf(1, "    decode[%x]: w=%d (%g)\n", dcode->idx, w, (w / 32.));

    /* each decoder processes width stream in parallel */
    zebra_symbol_type_t sym = dcode->type = ZEBRA_NONE;

#ifdef ENABLE_EAN
    if((dcode->ean.enable) &&
       (sym = _zebra_decode_ean(dcode)))
        dcode->type = sym;
#endif
#ifdef ENABLE_CODE39
    if(TEST_CFG(dcode->code39.config, ZEBRA_CFG_ENABLE) &&
       (sym = _zebra_decode_code39(dcode)) > ZEBRA_PARTIAL)
        dcode->type = sym;
#endif
#ifdef ENABLE_CODE128
    if(TEST_CFG(dcode->code128.config, ZEBRA_CFG_ENABLE) &&
       (sym = _zebra_decode_code128(dcode)) > ZEBRA_PARTIAL)
        dcode->type = sym;
#endif
#ifdef ENABLE_I25
    if(TEST_CFG(dcode->i25.config, ZEBRA_CFG_ENABLE) &&
       (sym = _zebra_decode_i25(dcode)) > ZEBRA_PARTIAL)
        dcode->type = sym;
#endif
#ifdef ENABLE_PDF417
    if(TEST_CFG(dcode->pdf417.config, ZEBRA_CFG_ENABLE) &&
       (sym = _zebra_decode_pdf417(dcode)) > ZEBRA_PARTIAL)
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

int zebra_decoder_set_config (zebra_decoder_t *dcode,
                              zebra_symbol_type_t sym,
                              zebra_config_t cfg,
                              int val)
{
    unsigned *config = NULL;
    switch(sym) {
    case ZEBRA_NONE:
        zebra_decoder_set_config(dcode, ZEBRA_EAN13, cfg, val);
        zebra_decoder_set_config(dcode, ZEBRA_EAN8, cfg, val);
        zebra_decoder_set_config(dcode, ZEBRA_UPCA, cfg, val);
        zebra_decoder_set_config(dcode, ZEBRA_UPCE, cfg, val);
        zebra_decoder_set_config(dcode, ZEBRA_ISBN10, cfg, val);
        zebra_decoder_set_config(dcode, ZEBRA_ISBN13, cfg, val);
        zebra_decoder_set_config(dcode, ZEBRA_I25, cfg, val);
        zebra_decoder_set_config(dcode, ZEBRA_CODE39, cfg, val);
        zebra_decoder_set_config(dcode, ZEBRA_CODE128, cfg, val);
        zebra_decoder_set_config(dcode, ZEBRA_PDF417, cfg, val);
        return(0);

#ifdef ENABLE_EAN
    case ZEBRA_EAN13:
        config = &dcode->ean.ean13_config;
        break;

    case ZEBRA_EAN8:
        config = &dcode->ean.ean8_config;
        break;

    case ZEBRA_UPCA:
        config = &dcode->ean.upca_config;
        break;

    case ZEBRA_UPCE:
        config = &dcode->ean.upce_config;
        break;

    case ZEBRA_ISBN10:
        config = &dcode->ean.isbn10_config;
        break;

    case ZEBRA_ISBN13:
        config = &dcode->ean.isbn13_config;
        break;
#endif

#ifdef ENABLE_I25
    case ZEBRA_I25:
        config = &dcode->i25.config;
        break;
#endif

#ifdef ENABLE_CODE39
    case ZEBRA_CODE39:
        config = &dcode->code39.config;
        break;
#endif

#ifdef ENABLE_CODE128
    case ZEBRA_CODE128:
        config = &dcode->code128.config;
        break;
#endif

#ifdef ENABLE_PDF417
    case ZEBRA_PDF417:
        config = &dcode->pdf417.config;
        break;
#endif

    /* FIXME handle addons */

    default:
        return(1);
    }
    if(!config || cfg >= ZEBRA_CFG_NUM)
        return(1);

    if(!val)
        *config &= ~(1 << cfg);
    else if(val == 1)
        *config |= (1 << cfg);
    else
        return(1);

#ifdef ENABLE_EAN
    dcode->ean.enable = TEST_CFG(dcode->ean.ean13_config |
                                 dcode->ean.ean8_config |
                                 dcode->ean.upca_config |
                                 dcode->ean.upce_config |
                                 dcode->ean.isbn10_config |
                                 dcode->ean.isbn13_config,
                                 ZEBRA_CFG_ENABLE);
#endif

    return(0);
}

static char *decoder_dump = NULL;

const char *_zebra_decoder_buf_dump (unsigned char *buf,
                                     unsigned int buflen)
{
    int dumplen = (buflen * 3) + 12;
    if(!decoder_dump || dumplen > strlen(decoder_dump)) {
        if(decoder_dump)
            free(decoder_dump);
        decoder_dump = malloc(dumplen);
    }
    char *p = decoder_dump +
        snprintf(decoder_dump, 12, "buf[%04x]=",
                 (buflen > 0xffff) ? 0xffff : buflen);
    int i;
    for(i = 0; i < buflen; i++)
        p += snprintf(p, 4, "%s%02x", (i) ? " " : "",  buf[i]);
    return(decoder_dump);
}
