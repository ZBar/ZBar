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
#include <assert.h>

#include <zebra.h>
#include "symbol.h"

const char *zebra_get_symbol_name (zebra_symbol_type_t sym)
{
    switch(sym & ZEBRA_SYMBOL) {
    case ZEBRA_EAN8: return("EAN-8");
    case ZEBRA_UPCE: return("UPC-E");
    case ZEBRA_UPCA: return("UPC-A");
    case ZEBRA_EAN13: return("EAN-13");
    case ZEBRA_CODE128: return("CODE-128");
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

zebra_symbol_type_t zebra_symbol_get_type (const zebra_symbol_t *sym)
{
    return(sym->type);
}

const char *zebra_symbol_get_data (const zebra_symbol_t *sym)
{
    return(sym->data);
}

int zebra_symbol_get_count (const zebra_symbol_t *sym)
{
    return(sym->cache_count);
}

unsigned zebra_symbol_get_loc_size (const zebra_symbol_t *sym)
{
    return(sym->npts);
}

int zebra_symbol_get_loc_x (const zebra_symbol_t *sym,
                            unsigned idx)
{
    if(idx < sym->npts)
        return(sym->pts[idx].x);
    else
        return(-1);
}

int zebra_symbol_get_loc_y (const zebra_symbol_t *sym,
                            unsigned idx)
{
    if(idx < sym->npts)
        return(sym->pts[idx].y);
    else
        return(-1);
}

const zebra_symbol_t *zebra_symbol_next (const zebra_symbol_t *sym)
{
    return((sym) ? sym->next : NULL);
}
