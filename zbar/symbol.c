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
    case ZBAR_CODE39: return("CODE-39");
    case ZBAR_CODE128: return("CODE-128");
    case ZBAR_PDF417: return("PDF417");
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

zbar_symbol_type_t zbar_symbol_get_type (const zbar_symbol_t *sym)
{
    return(sym->type);
}

const char *zbar_symbol_get_data (const zbar_symbol_t *sym)
{
    return(sym->data);
}

int zbar_symbol_get_count (const zbar_symbol_t *sym)
{
    return(sym->cache_count);
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

const zbar_symbol_t *zbar_symbol_next (const zbar_symbol_t *sym)
{
    return((sym) ? sym->next : NULL);
}


static const char *xmlfmt[] = {
    "<symbol type='%s'",
    " count='%d'",
    "><data><![CDATA[",
    "]]></data></symbol>",
};

/* FIXME suspect... */
#define MAX_INT_DIGITS 10

static inline void growbuf (unsigned **buf,
                            unsigned *len,
                            unsigned newlen)
{
}

char *zbar_symbol_xml (const zbar_symbol_t *sym,
                       char **buf,
                       unsigned *len)
{
    const char *type = zbar_get_symbol_name(sym->type);
    unsigned datalen = strlen(sym->data);
    unsigned maxlen = (strlen(xmlfmt[0]) + strlen(xmlfmt[1]) +
                       strlen(xmlfmt[2]) + strlen(xmlfmt[3]) +
                       strlen(type) + datalen + MAX_INT_DIGITS + 1);
    if(!*buf || (*len < maxlen)) {
        if(*buf)
            free(*buf);
        *buf = malloc(maxlen);
        /* FIXME check OOM */
        *len = maxlen;
    }

    int n = snprintf(*buf, maxlen, xmlfmt[0], type);
    assert(n > 0);
    assert(n <= maxlen);

    if(sym->cache_count) {
        int i = snprintf(*buf + n, maxlen - n, xmlfmt[1], sym->cache_count);
        assert(i > 0);
        n += i;
        assert(n <= maxlen);
    }

    int i = strlen(xmlfmt[2]);
    memcpy(*buf + n, xmlfmt[2], i + 1);
    n += i;
    assert(n <= maxlen);

    /* FIXME binary data */
    /* FIXME handle "]]>" */
    strncpy(*buf + n, sym->data, datalen + 1);
    n += datalen;
    assert(n <= maxlen);

    i = strlen(xmlfmt[3]);
    memcpy(*buf + n, xmlfmt[3], i + 1);
    n += i;
    assert(n <= maxlen);

    *len = n;
    return(*buf);
}
