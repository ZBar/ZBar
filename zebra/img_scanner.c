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
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef DEBUG_IMG_SCANNER
# include <stdio.h>     /* fprintf */
#endif
#include <stdlib.h>     /* malloc, free */
#include <string.h>     /* strlen, strcmp, memset, memcpy */
#include <assert.h>

#include "zebra.h"
#include "symbol.h"

#ifdef DEBUG_IMG_SCANNER
# define dprintf(...) \
    fprintf(stderr, __VA_ARGS__)
# define ASSERT_POS \
    assert(iscn->p == img + iscn->x + iscn->y * iscn->w);
#else
# define dprintf(...)
#define ASSERT_POS
#endif

/* image scanner state */
struct zebra_img_scanner_s {
    zebra_scanner_t *scn;       /* associated linear intensity scanner */
    zebra_decoder_t *dcode;     /* associated symbol decoder */

    unsigned w, h;              /* configured width and height */
    void *userdata;             /* application data */

    int x, y;                   /* current image location */
    const unsigned char *p;

    int nsyms;                  /* current syms entries */
    unsigned symslen;           /* allocated size of syms */
    zebra_symbol_t *syms;       /* decoded symbol data */
};

static void symbol_handler (zebra_decoder_t *dcode)
{
    zebra_img_scanner_t *iscn = zebra_decoder_get_userdata(dcode);
    assert(iscn->dcode == dcode);

    zebra_symbol_type_t type = zebra_decoder_get_type(dcode);
    /* FIXME assert(type == ZEBRA_PARTIAL) */
    /* FIXME debug flag to save/display all PARTIALs */
    if(type <= ZEBRA_PARTIAL)
        return;

    /* FIXME check if (x, y) is inside existing polygon */

    zebra_symbol_t *sym = &iscn->syms[iscn->nsyms];
    sym->npts = 0;
    /* FIXME extract bars from image and decode */

    if(++sym->npts >= sym->ptslen)
        sym->pts = realloc(sym->pts, ++sym->ptslen * sizeof(point_t));
    sym->pts[0].x = iscn->x;
    sym->pts[0].y = iscn->y;

    const char *data = zebra_decoder_get_data(dcode);
    int i;
    for(i = 0; i < iscn->nsyms; i++) {
        zebra_symbol_t *isym = &iscn->syms[i];
        if(isym->type == type &&
           !strcmp(isym->data, data)) {
            isym->refcnt++;
            return;
        }
    }

    /* save new symbol */
    int datalen = strlen(data) + 1;
    if(sym->datalen < datalen) {
        if(sym->data)
            free(sym->data);
        sym->data = malloc(datalen);
        sym->datalen = datalen;
    }
    sym->type = type;
    memcpy(sym->data, data, datalen);
    sym->refcnt = 1;

    if(++iscn->nsyms >= iscn->symslen) {
        iscn->symslen++;
        assert(iscn->nsyms < iscn->symslen);
        iscn->syms = realloc(iscn->syms,
                             iscn->symslen * sizeof(zebra_symbol_t));
        sym = &iscn->syms[iscn->nsyms];
        memset(sym, 0, sizeof(zebra_symbol_t));
    }
}

zebra_img_scanner_t *zebra_img_scanner_create ()
{
    zebra_img_scanner_t *iscn = calloc(1, sizeof(zebra_img_scanner_t));
    assert(iscn);
    iscn->dcode = zebra_decoder_create();
    zebra_decoder_set_userdata(iscn->dcode, iscn);
    zebra_decoder_set_handler(iscn->dcode, symbol_handler);
    iscn->scn = zebra_scanner_create(iscn->dcode);
    iscn->symslen = 1;
    iscn->syms = calloc(1, sizeof(zebra_symbol_t));
    return(iscn);
}

void zebra_img_scanner_destroy (zebra_img_scanner_t *iscn)
{
    assert(iscn);
    zebra_scanner_destroy(iscn->scn);
    zebra_decoder_destroy(iscn->dcode);
    free(iscn);
}

void zebra_img_scanner_set_userdata (zebra_img_scanner_t *iscn,
                                     void *userdata)
{
    assert(iscn);
    iscn->userdata = userdata;
}

void *zebra_img_scanner_get_userdata (const zebra_img_scanner_t *iscn)
{
    assert(iscn);
    return(iscn->userdata);
}

void zebra_img_scanner_set_size (zebra_img_scanner_t *iscn,
                                 unsigned width,
                                 unsigned height)
{
    assert(iscn);
    iscn->w = width;
    iscn->h = height;
}

unsigned char
zebra_img_scanner_get_result_size (const zebra_img_scanner_t *iscn)
{
    assert(iscn);
    return(iscn->nsyms);
}

zebra_symbol_t*
zebra_img_scanner_get_result (const zebra_img_scanner_t *iscn,
                              unsigned char idx)
{
    assert(iscn);
    if(idx < iscn->nsyms)
        return(&iscn->syms[idx]);
    else
        return(NULL);
}


static inline void quiet_border (zebra_img_scanner_t *iscn,
                                 unsigned samples)
{
    unsigned i;
    for(i = samples; i; i--)
        zebra_scan_y(iscn->scn, 0);
    zebra_scanner_new_scan(iscn->scn);
}

static inline void movedelta (zebra_img_scanner_t *iscn,
                              int dx,
                              int dy)
{
    iscn->x += dx;
    iscn->y += dy;
    iscn->p += dx + dy * iscn->w;
}

int zebra_img_scan_y (zebra_img_scanner_t *iscn,
                      const void *img)
{
    int quiet;
    if(!iscn || !iscn->w || !iscn->h)
        return(-1);

    assert(iscn->symslen);
    iscn->nsyms = 0;

    /* FIXME add density config api (calc optimal default) */
    quiet = iscn->w / 32;
    if(quiet < 8)
        quiet = 8;

    zebra_scanner_new_scan(iscn->scn);

    iscn->x = iscn->y = 0;
    iscn->p = img;
    for(movedelta(iscn, 0, 8); iscn->y < iscn->h; movedelta(iscn, 1, 16)) {
        dprintf("img_x+: %03x,%03x (%p)\n", iscn->x, iscn->y, iscn->p);
        for(; iscn->x < iscn->w; movedelta(iscn, 1, 0)) {
            ASSERT_POS;
            zebra_scan_y(iscn->scn, *iscn->p);
        }
        quiet_border(iscn, quiet);

        movedelta(iscn, -1, 16);
        if(iscn->y >= iscn->h)
            break;

        dprintf("img_x-: %03x,%03x (%p)\n", iscn->x, iscn->y, iscn->p);
        for(; iscn->x > 0; movedelta(iscn, -1, 0)) {
            ASSERT_POS;
            zebra_scan_y(iscn->scn, *iscn->p);
        }
        quiet_border(iscn, quiet);
    }

    iscn->x = iscn->y = 0;
    iscn->p = img;
    for(movedelta(iscn, 8, 0); iscn->x < iscn->w; movedelta(iscn, 16, 1)) {
        dprintf("img_y+: %03x,%03x (%p)\n", iscn->x, iscn->y, iscn->p);
        for(; iscn->y < iscn->h; movedelta(iscn, 0, 1)) {
            ASSERT_POS;
            zebra_scan_y(iscn->scn, *iscn->p);
        }
        quiet_border(iscn, quiet);

        movedelta(iscn, 16, -1);
        if(iscn->x >= iscn->w)
            break;

        dprintf("img_y-: %03x,%03x (%p)\n", iscn->x, iscn->y, iscn->p);
        for(; iscn->y >= 0; movedelta(iscn, 0, -1)) {
            ASSERT_POS;
            zebra_scan_y(iscn->scn, *iscn->p);
        }
        quiet_border(iscn, quiet);
    }

    /* flush scanner pipe */
    zebra_scanner_new_scan(iscn->scn);

    assert(iscn->nsyms < iscn->symslen);
    return(iscn->nsyms);
}
