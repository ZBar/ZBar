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
#include <assert.h>

#include <zebra.h>
#include "symbol.h"

zebra_symbol_type_t zebra_symbol_get_type (zebra_symbol_t *sym)
{
    assert(sym);
    return(sym->type);
}

const char *zebra_symbol_get_data (zebra_symbol_t *sym)
{
    assert(sym);
    return(sym->data);
}

unsigned char zebra_symbol_get_loc_size (zebra_symbol_t *sym)
{
    assert(sym);
    return(sym->npts);
}

int zebra_symbol_get_loc_x (zebra_symbol_t *sym,
                            unsigned char idx)
{
    assert(sym);
    if(idx < sym->npts)
        return(sym->pts[idx].x);
    else
        return(-1);
}

int zebra_symbol_get_loc_y (zebra_symbol_t *sym,
                            unsigned char idx)
{
    assert(sym);
    if(idx < sym->npts)
        return(sym->pts[idx].y);
    else
        return(-1);
}
