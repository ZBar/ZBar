/*------------------------------------------------------------------------
 *  Copyright 2007-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
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
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <zbar.h>
#include "symbol.h"

const char *zbar_get_symbol_name (zbar_symbol_type_t sym)
{
    switch(sym & ZBAR_SYMBOL) {
    case ZBAR_EAN8: return("EAN-8");
    case ZBAR_UPCE: return("UPC-E");
    case ZBAR_ISBN10: return("ISBN-10");
    case ZBAR_UPCA: return("UPC-A");
    case ZBAR_EAN13: return("EAN-13");
    case ZBAR_ISBN13: return("ISBN-13");
    case ZBAR_I25: return("I2/5");
    case ZBAR_DATABAR: return("DataBar");
    case ZBAR_CODE39: return("CODE-39");
    case ZBAR_CODE128: return("CODE-128");
    case ZBAR_PDF417: return("PDF417");
    case ZBAR_QRCODE: return("QR-Code");
    default: return("UNKNOWN");
    }
}

const char *zbar_get_addon_name (zbar_symbol_type_t sym)
{
    switch(sym & ZBAR_ADDON) {
    case ZBAR_ADDON2: return("+2");
    case ZBAR_ADDON5: return("+5");
    default: return("");
    }
}

const char *zbar_get_orientation_name (zbar_orientation_t orient)
{
    switch(orient) {
    case ZBAR_ORIENT_UP: return("UP");
    case ZBAR_ORIENT_RIGHT: return("RIGHT");
    case ZBAR_ORIENT_DOWN: return("DOWN");
    case ZBAR_ORIENT_LEFT: return("LEFT");
    default: return("UNKNOWN");
    }
}

void _zbar_symbol_free (zbar_symbol_t *sym)
{
    if(sym->syms) {
        zbar_symbol_set_ref(sym->syms, -1);
        sym->syms = NULL;
    }
    if(sym->pts)
        free(sym->pts);
    if(sym->data_alloc && sym->data)
        free(sym->data);
    free(sym);
}

void zbar_symbol_ref (const zbar_symbol_t *sym,
                      int refs)
{
    zbar_symbol_t *ncsym = (zbar_symbol_t*)sym;
    _zbar_symbol_refcnt(ncsym, refs);
}

zbar_symbol_type_t zbar_symbol_get_type (const zbar_symbol_t *sym)
{
    return(sym->type);
}

const char *zbar_symbol_get_data (const zbar_symbol_t *sym)
{
    return(sym->data);
}

unsigned int zbar_symbol_get_data_length (const zbar_symbol_t *sym)
{
    return(sym->datalen);
}

int zbar_symbol_get_count (const zbar_symbol_t *sym)
{
    return(sym->cache_count);
}

int zbar_symbol_get_quality (const zbar_symbol_t *sym)
{
    return(sym->quality);
}

unsigned zbar_symbol_get_loc_size (const zbar_symbol_t *sym)
{
    return(sym->npts);
}

int zbar_symbol_get_loc_x (const zbar_symbol_t *sym,
                           unsigned idx)
{
    if(idx < sym->npts)
        return(sym->pts[idx].x);
    else
        return(-1);
}

int zbar_symbol_get_loc_y (const zbar_symbol_t *sym,
                           unsigned idx)
{
    if(idx < sym->npts)
        return(sym->pts[idx].y);
    else
        return(-1);
}

zbar_orientation_t zbar_symbol_get_orientation (const zbar_symbol_t *sym)
{
    return(sym->orient);
}

const zbar_symbol_t *zbar_symbol_next (const zbar_symbol_t *sym)
{
    return((sym) ? sym->next : NULL);
}

const zbar_symbol_set_t*
zbar_symbol_get_components (const zbar_symbol_t *sym)
{
    return(sym->syms);
}

const zbar_symbol_t *zbar_symbol_first_component (const zbar_symbol_t *sym)
{
    return((sym && sym->syms) ? sym->syms->head : NULL);
}


unsigned base64_encode (char *dst,
                        const char *src,
                        unsigned srclen)
{
    static const char alphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    char *start = dst;
    int nline = 19;
    for(; srclen; srclen -= 3) {
        unsigned int buf = *(src++) << 16;
        if(srclen > 1) buf |= *(src++) << 8;
        if(srclen > 2) buf |= *(src++);
        *(dst++) = alphabet[(buf >> 18) & 0x3f];
        *(dst++) = alphabet[(buf >> 12) & 0x3f];
        *(dst++) = (srclen > 1) ? alphabet[(buf >> 6) & 0x3f] : '=';
        *(dst++) = (srclen > 2) ? alphabet[buf & 0x3f] : '=';
        if(srclen < 3) break;
        if(!--nline) { *(dst++) = '\n'; nline = 19; }
    }
    *(dst++) = '\n';
    *(dst++) = '\0';
    return(dst - start - 1);
}

static const char *xmlfmt[] = {
    "<symbol type='%s' quality='%d' orientation='%s'",
    " count='%d'",
    "><data",
    " format='base64' length='%d'",
    "><![CDATA[",
    "\n",
    "]]></data></symbol>",
};

/* FIXME suspect... */
#define MAX_INT_DIGITS 10

#define TMPL_COPY(t) do {                        \
        i = strlen(xmlfmt[(t)]);                 \
        memcpy(*buf + n, xmlfmt[(t)], i + 1);    \
        n += i;                                  \
        assert(n <= maxlen);                     \
    } while(0)

#define TMPL_FMT(t, ...) do {                                         \
        i = snprintf(*buf + n, maxlen - n, xmlfmt[(t)], __VA_ARGS__); \
        assert(i > 0);                                                \
        n += i;                                                       \
        assert(n <= maxlen);                                          \
    } while(0)

char *zbar_symbol_xml (const zbar_symbol_t *sym,
                       char **buf,
                       unsigned *len)
{
    unsigned int datalen, maxlen;
    int i, n = 0;

    const char *type = zbar_get_symbol_name(sym->type);
    const char *orient = zbar_get_orientation_name(sym->orient);

    /* check for binary data */
    unsigned char *data = (unsigned char*)sym->data;
    char binary = ((data[0] == 0xff && data[1] == 0xfe) ||
                   (data[0] == 0xfe && data[1] == 0xff) ||
                   !strncmp(sym->data, "<?xml", 5));
    for(i = 0; !binary && i < sym->datalen; i++) {
        char c = sym->data[i];
        binary = ((c < 0x20 && ((~0x00002600 >> c) & 1)) ||
                  (c == ']' && i + 2 < sym->datalen &&
                   sym->data[i + 1] == ']' &&
                   sym->data[i + 2] == '>'));
    }

    datalen = strlen(sym->data);
    if(binary)
        datalen = (sym->datalen + 2) / 3 * 4 + sym->datalen / 57 + 3;

    maxlen = (strlen(xmlfmt[0]) + strlen(xmlfmt[1]) +
              strlen(xmlfmt[2]) + strlen(xmlfmt[4]) +
              strlen(xmlfmt[6]) + strlen(type) + strlen(orient) +
              datalen + MAX_INT_DIGITS + 1);
    if(binary)
        maxlen += (strlen(xmlfmt[3]) + strlen(xmlfmt[5]) + MAX_INT_DIGITS);

    if(!*buf || (*len < maxlen)) {
        if(*buf)
            free(*buf);
        *buf = malloc(maxlen);
        /* FIXME check OOM */
        *len = maxlen;
    }

    TMPL_FMT(0, type, sym->quality, orient);

    if(sym->cache_count)
        TMPL_FMT(1, sym->cache_count);

    TMPL_COPY(2);
    if(binary)
        TMPL_FMT(3, sym->datalen);
    TMPL_COPY(4);

    if(!binary) {
        memcpy(*buf + n, sym->data, sym->datalen + 1);
        n += sym->datalen;
    }
    else {
        TMPL_COPY(5);
        n += base64_encode(*buf + n, sym->data, sym->datalen);
    }
    assert(n <= maxlen);

    TMPL_COPY(6);

    *len = n;
    return(*buf);
}


zbar_symbol_set_t *_zbar_symbol_set_create ()
{
    zbar_symbol_set_t *syms = calloc(1, sizeof(*syms));
    _zbar_refcnt(&syms->refcnt, 1);
    return(syms);
}

inline void _zbar_symbol_set_free (zbar_symbol_set_t *syms)
{
    zbar_symbol_t *sym, *next;
    for(sym = syms->head; sym; sym = next) {
        next = sym->next;
        sym->next = NULL;
        _zbar_symbol_refcnt(sym, -1);
    }
    syms->head = NULL;
    free(syms);
}

void zbar_symbol_set_ref (const zbar_symbol_set_t *syms,
                          int delta)
{
    zbar_symbol_set_t *ncsyms = (zbar_symbol_set_t*)syms;
    if(!_zbar_refcnt(&ncsyms->refcnt, delta) && delta <= 0)
        _zbar_symbol_set_free(ncsyms);
}

int zbar_symbol_set_get_size (const zbar_symbol_set_t *syms)
{
    return(syms->nsyms);
}

const zbar_symbol_t*
zbar_symbol_set_first_symbol (const zbar_symbol_set_t *syms)
{
    zbar_symbol_t *sym = syms->tail;
    if(sym)
        return(sym->next);
    return(syms->head);
}

const zbar_symbol_t*
zbar_symbol_set_first_unfiltered (const zbar_symbol_set_t *syms)
{
    return(syms->head);
}
