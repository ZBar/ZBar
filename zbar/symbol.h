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

typedef struct point_s {
    int x, y;
} point_t;

struct zbar_symbol_s {
    zbar_symbol_type_t type;    /* symbol type */
    unsigned int datalen;       /* allocation size of data */
    char *data;                 /* ascii symbol data */

    unsigned ptslen;            /* allocation size of pts */
    unsigned npts;              /* number of points in location polygon */
    point_t *pts;               /* list of points in location polygon */

    zbar_symbol_t *next;        /* linked list of results */
    unsigned long time;         /* relative symbol capture time */
    int cache_count;            /* cache state */
};

static inline void sym_add_point (zbar_symbol_t *sym,
                                  int x,
                                  int y)
{
    int i = sym->npts;
    if(++sym->npts >= sym->ptslen)
        sym->pts = realloc(sym->pts, ++sym->ptslen * sizeof(point_t));
    sym->pts[i].x = x;
    sym->pts[i].y = y;
}

static inline void sym_destroy (zbar_symbol_t *sym)
{
    if(sym->pts)
        free(sym->pts);
    if(sym->data)
        free(sym->data);
    free(sym);
}

#endif
