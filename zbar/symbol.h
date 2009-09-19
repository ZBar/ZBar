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
#ifndef _SYMBOL_H_
#define _SYMBOL_H_

#include <stdlib.h>
#include <zbar.h>
#include "refcnt.h"

typedef struct point_s {
    int x, y;
} point_t;

struct zbar_symbol_s {
    zbar_symbol_type_t type;    /* symbol type */
    unsigned int data_alloc;    /* allocation size of data */
    unsigned int datalen;       /* length of binary symbol data */
    char *data;                 /* symbol data */

    unsigned pts_alloc;         /* allocation size of pts */
    unsigned npts;              /* number of points in location polygon */
    point_t *pts;               /* list of points in location polygon */

    refcnt_t refcnt;            /* reference count */
    zbar_symbol_t *next;        /* linked list of results (or siblings) */
    zbar_symbol_t *syms;        /* first component of composite result */
    unsigned long time;         /* relative symbol capture time */
    int cache_count;            /* cache state */
    int quality;                /* relative symbol reliability metric */
};

static inline void sym_add_point (zbar_symbol_t *sym,
                                  int x,
                                  int y)
{
    int i = sym->npts;
    if(++sym->npts >= sym->pts_alloc)
        sym->pts = realloc(sym->pts, ++sym->pts_alloc * sizeof(point_t));
    sym->pts[i].x = x;
    sym->pts[i].y = y;
}

static inline void sym_destroy (zbar_symbol_t *sym)
{
    if(sym->pts)
        free(sym->pts);
    if(sym->data_alloc && sym->data)
        free(sym->data);
    free(sym);
}

static inline void _zbar_symbol_refcnt (zbar_symbol_t *sym,
                                        int delta)
{
    if(!_zbar_refcnt(&sym->refcnt, delta) && delta <= 0)
        sym_destroy(sym);
}

#endif
