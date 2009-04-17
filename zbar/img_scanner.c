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
#include <unistd.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <stdlib.h>     /* malloc, free */
#include <time.h>       /* clock_gettime */
#include <sys/time.h>   /* gettimeofday */
#include <string.h>     /* strlen, strcmp, memset, memcpy */
#include <assert.h>

#include <zbar.h>
#include "error.h"
#include "image.h"

#if 1
# define ASSERT_POS \
    assert(p == data + x + y * w)
#else
# define ASSERT_POS
#endif

/* FIXME cache setting configurability */

/* number of times the same result must be detected
 * in "nearby" images before being reported
 */
#define CACHE_CONSISTENCY    3 /* images */

/* time interval for which two images are considered "nearby"
 */
#define CACHE_PROXIMITY   1000 /* ms */

/* time that a result must *not* be detected before
 * it will be reported again
 */
#define CACHE_HYSTERESIS  2000 /* ms */

/* time after which cache entries are invalidated
 */
#define CACHE_TIMEOUT     (CACHE_HYSTERESIS * 2) /* ms */

#define NUM_SCN_CFGS (ZBAR_CFG_Y_DENSITY - ZBAR_CFG_X_DENSITY + 1)

#define CFG(iscn, cfg) ((iscn)->configs[(cfg) - ZBAR_CFG_X_DENSITY])


/* image scanner state */
struct zbar_image_scanner_s {
    zbar_scanner_t *scn;        /* associated linear intensity scanner */
    zbar_decoder_t *dcode;      /* associated symbol decoder */

    const void *userdata;       /* application data */
    /* user result callback */
    zbar_image_data_handler_t *handler;

    zbar_image_t *img;          /* currently scanning image *root* */
    int nsyms;                  /* total cached symbols */
    zbar_symbol_t *syms;        /* recycled symbols */

    int enable_cache;           /* current result cache state */
    zbar_symbol_t *cache;       /* inter-image result cache entries */

    /* configuration settings */
    int configs[NUM_SCN_CFGS];
};

static inline void recycle_syms (zbar_image_scanner_t *iscn,
                                 zbar_image_t *img)
{
    /* walk to root of clone tree */
    while(img) {
        img->nsyms = 0;
        zbar_symbol_t *sym = img->syms;
        if(sym) {
            /* recycle image symbols */
            iscn->nsyms++;
            while(sym->next) {
                sym = sym->next;
                iscn->nsyms++;
            }
            sym->next = iscn->syms;
            iscn->syms = img->syms;
            img->syms = NULL;
        }
        /* save root */
        iscn->img = img;
        img = img->next;
    }
}

static inline zbar_symbol_t *alloc_sym (zbar_image_scanner_t *iscn,
                                        zbar_symbol_type_t type,
                                        const char *data)
{
    /* recycle old or alloc new symbol */
    zbar_symbol_t *sym = iscn->syms;
    if(sym) {
        iscn->syms = sym->next;
        assert(iscn->nsyms);
        iscn->nsyms--;
    }
    else {
        sym = calloc(1, sizeof(zbar_symbol_t));
        assert(!iscn->nsyms);
    }

    /* save new symbol data */
    sym->type = type;
    int datalen = strlen(data) + 1;
    if(sym->datalen < datalen) {
        if(sym->data)
            free(sym->data);
        sym->data = malloc(datalen);
        sym->datalen = datalen;
    }
    memcpy(sym->data, data, datalen);

    return(sym);
}

static inline zbar_symbol_t *cache_lookup (zbar_image_scanner_t *iscn,
                                           zbar_symbol_t *sym)
{
    /* search for matching entry in cache */
    zbar_symbol_t **entry = &iscn->cache;
    while(*entry) {
        if((*entry)->type == sym->type &&
           !strcmp((*entry)->data, sym->data))
            break;
        if((sym->time - (*entry)->time) > CACHE_TIMEOUT) {
            /* recycle stale cache entry */
            zbar_symbol_t *next = (*entry)->next;
            (*entry)->next = iscn->syms;
            iscn->syms = *entry;
            iscn->nsyms++;
            *entry = next;
        }
        else
            entry = &(*entry)->next;
    }
    return(*entry);
}

static void symbol_handler (zbar_image_scanner_t *iscn,
                            int x,
                            int y)
{
    zbar_symbol_type_t type = zbar_decoder_get_type(iscn->dcode);
    /* FIXME assert(type == ZBAR_PARTIAL) */
    /* FIXME debug flag to save/display all PARTIALs */
    if(type <= ZBAR_PARTIAL)
        return;

    const char *data = zbar_decoder_get_data(iscn->dcode);

    /* FIXME deprecate - instead check (x, y) inside existing polygon */
    zbar_symbol_t *sym;
    for(sym = iscn->img->syms; sym; sym = sym->next)
        if(sym->type == type &&
           !strcmp(sym->data, data)) {
            /* add new point to existing set */
            /* FIXME should be polygon */
            sym_add_point(sym, x, y);
            return;
        }

    sym = alloc_sym(iscn, type, data);

    /* timestamp symbol */
#if _POSIX_TIMERS > 0
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    sym->time = (abstime.tv_sec * 1000) + ((abstime.tv_nsec / 500000) + 1) / 2;
#else
    struct timeval abstime;
    gettimeofday(&abstime, NULL);
    sym->time = (abstime.tv_sec * 1000) + ((abstime.tv_usec / 500) + 1) / 2;
#endif

    /* initialize first point */
    sym->npts = 0;
    sym_add_point(sym, x, y);

    /* attach to current root image */
    sym->next = iscn->img->syms;
    iscn->img->syms = sym;
    iscn->img->nsyms++;

    if(iscn->enable_cache) {
        zbar_symbol_t *entry = cache_lookup(iscn, sym);
        if(!entry) {
            entry = alloc_sym(iscn, sym->type, sym->data);
            entry->time = sym->time - CACHE_HYSTERESIS;
            entry->cache_count = -CACHE_CONSISTENCY;
            /* add to cache */
            entry->next = iscn->cache;
            iscn->cache = entry;
        }

        /* consistency check and hysteresis */
        uint32_t age = sym->time - entry->time;
        entry->time = sym->time;
        int near = (age < CACHE_PROXIMITY);
        int far = (age >= CACHE_HYSTERESIS);
        int dup = (entry->cache_count >= 0);
        if((!dup && !near) || far)
            entry->cache_count = -CACHE_CONSISTENCY;
        else if(dup || near)
            entry->cache_count++;

        sym->cache_count = entry->cache_count;
    }
    else
        sym->cache_count = 0;

    if(iscn->handler)
        iscn->handler(iscn->img, iscn->userdata);
}

zbar_image_scanner_t *zbar_image_scanner_create ()
{
    zbar_image_scanner_t *iscn = calloc(1, sizeof(zbar_image_scanner_t));
    if(!iscn)
        return(NULL);
    iscn->dcode = zbar_decoder_create();
    iscn->scn = zbar_scanner_create(iscn->dcode);
    if(!iscn->dcode || !iscn->scn) {
        zbar_image_scanner_destroy(iscn);
        return(NULL);
    }

    /* apply default configuration */
    CFG(iscn, ZBAR_CFG_X_DENSITY) = 16;
    CFG(iscn, ZBAR_CFG_Y_DENSITY) = 16;
    return(iscn);
}

void zbar_image_scanner_destroy (zbar_image_scanner_t *iscn)
{
    if(iscn->scn)
        zbar_scanner_destroy(iscn->scn);
    iscn->scn = NULL;
    if(iscn->dcode)
        zbar_decoder_destroy(iscn->dcode);
    iscn->dcode = NULL;
    while(iscn->syms) {
        zbar_symbol_t *next = iscn->syms->next;
        sym_destroy(iscn->syms);
        iscn->syms = next;
    }
    free(iscn);
}

zbar_image_data_handler_t*
zbar_image_scanner_set_data_handler (zbar_image_scanner_t *iscn,
                                     zbar_image_data_handler_t *handler,
                                     const void *userdata)
{
    zbar_image_data_handler_t *result = iscn->handler;
    iscn->handler = handler;
    iscn->userdata = userdata;
    return(result);
}

int zbar_image_scanner_set_config (zbar_image_scanner_t *iscn,
                                   zbar_symbol_type_t sym,
                                   zbar_config_t cfg,
                                   int val)
{
    if(cfg >= ZBAR_CFG_X_DENSITY && cfg <= ZBAR_CFG_Y_DENSITY) {
        if(sym > ZBAR_PARTIAL)
            return(1);

        CFG(iscn, cfg) = val;
        return(0);
    }
    return(zbar_decoder_set_config(iscn->dcode, sym, cfg, val));
}

void zbar_image_scanner_enable_cache(zbar_image_scanner_t *iscn,
                                     int enable)
{
    if(iscn->cache) {
        /* recycle all cached syms */
        zbar_symbol_t *entry;
        for(entry = iscn->cache; entry->next; entry = entry->next)
            iscn->nsyms++;
        iscn->nsyms++;
        entry->next = iscn->syms;
        iscn->syms = iscn->cache;
        iscn->cache = NULL;
    }
    iscn->enable_cache = enable;
}

static inline void quiet_border (zbar_image_scanner_t *iscn,
                                 unsigned samples,
                                 int x,
                                 int y)
{
    unsigned i;
    for(i = samples; i; i--)
        if(zbar_scan_y(iscn->scn, 255))
            symbol_handler(iscn, x, y);
    /* flush final transition with 2 black pixels */
    for(i = 2; i; i--)
        if(zbar_scan_y(iscn->scn, 0))
            symbol_handler(iscn, x, y);
    if(zbar_scanner_new_scan(iscn->scn))
        symbol_handler(iscn, x, y);
}

#define movedelta(dx, dy) do {                  \
        x += (dx);                              \
        y += (dy);                              \
        p += (dx) + ((intptr_t)(dy) * w);       \
    } while(0);

int zbar_scan_image (zbar_image_scanner_t *iscn,
                     zbar_image_t *img)
{
    recycle_syms(iscn, img);

    /* get grayscale image, convert if necessary */
    img = zbar_image_convert(img, fourcc('Y','8','0','0'));
    if(!img)
        return(-1);

    unsigned w = zbar_image_get_width(img);
    unsigned h = zbar_image_get_height(img);
    const uint8_t *data = zbar_image_get_data(img);

    /* FIXME less arbitrary lead-out default */
    int quiet = w / 32;
    if(quiet < 8)
        quiet = 8;

    int density = CFG(iscn, ZBAR_CFG_Y_DENSITY);
    if(density > 0) {
        const uint8_t *p = data;
        int x = 0, y = 0;

        int border = (((h - 1) % density) + 1) / 2;
        if(border > h / 2)
            border = h / 2;
        movedelta(0, border);

        if(zbar_scanner_new_scan(iscn->scn))
            symbol_handler(iscn, x, y);

        while(y < h) {
            zprintf(32, "img_x+: %03x,%03x @%p\n", x, y, p);
            while(x < w) {
                ASSERT_POS;
                if(zbar_scan_y(iscn->scn, *p))
                    symbol_handler(iscn, x, y);
                movedelta(1, 0);
            }
            quiet_border(iscn, quiet, x, y);

            movedelta(-1, density);
            if(y >= h)
                break;

            zprintf(32, "img_x-: %03x,%03x @%p\n", x, y, p);
            while(x > 0) {
                ASSERT_POS;
                if(zbar_scan_y(iscn->scn, *p))
                    symbol_handler(iscn, x, y);
                movedelta(-1, 0);
            }
            quiet_border(iscn, quiet, x, y);

            movedelta(1, density);
        }
    }

    density = CFG(iscn, ZBAR_CFG_X_DENSITY);
    if(density > 0) {
        const uint8_t *p = data;
        int x = 0, y = 0;

        int border = (((w - 1) % density) + 1) / 2;
        if(border > w / 2)
            border = w / 2;
        movedelta(border, 0);

        while(x < w) {
            zprintf(32, "img_y+: %03x,%03x @%p\n", x, y, p);
            while(y < h) {
                ASSERT_POS;
                if(zbar_scan_y(iscn->scn, *p))
                    symbol_handler(iscn, x, y);
                movedelta(0, 1);
            }
            quiet_border(iscn, quiet, x, y);

            movedelta(density, -1);
            if(x >= w)
                break;

            zprintf(32, "img_y-: %03x,%03x @%p\n", x, y, p);
            while(y >= 0) {
                ASSERT_POS;
                if(zbar_scan_y(iscn->scn, *p))
                    symbol_handler(iscn, x, y);
                movedelta(0, -1);
            }
            quiet_border(iscn, quiet, x, y);

            movedelta(density, 1);
        }
    }

    /* release reference to converted image */
    zbar_image_destroy(img);
    return(iscn->img->nsyms);
}
