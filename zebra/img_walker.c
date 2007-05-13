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
#include <inttypes.h>
#ifdef DEBUG_IMG_WALKER
# include <stdio.h>     /* fprintf */
#endif
#include <stdlib.h>     /* malloc, free */
#include <assert.h>

#include "zebra.h"

#ifdef DEBUG_IMG_WALKER
# define dprintf(...) \
    fprintf(stderr, __VA_ARGS__)
#else
# define dprintf(...)
#endif

/* image walker state */
struct zebra_img_walker_s {
    unsigned x, y;      /* current column and row */
    unsigned w, h;      /* configured width and height */
    unsigned dx, dy;    /* configured bytes_per_col and bytes_per_row */
    void *userdata;     /* application data */
    zebra_img_walker_handler_t *handler;
};

zebra_img_walker_t *zebra_img_walker_create ()
{
    zebra_img_walker_t *walk = malloc(sizeof(zebra_img_walker_t));
    return(walk);
}

void zebra_img_walker_destroy (zebra_img_walker_t *walk)
{
    free(walk);
}

void zebra_img_walker_set_handler (zebra_img_walker_t *walk,
                                   zebra_img_walker_handler_t *handler)
{
    walk->handler = handler;
}

void zebra_img_walker_set_userdata (zebra_img_walker_t *walk,
                                    void *userdata)
{
    walk->userdata = userdata;
}

void *zebra_img_walker_get_userdata (zebra_img_walker_t *walk)
{
    return(walk->userdata);
}

void zebra_img_walker_set_size (zebra_img_walker_t *walk,
                                unsigned width,
                                unsigned height)
{
    walk->w = width;
    walk->h = height;
}

void zebra_img_walker_set_stride (zebra_img_walker_t *walk,
                                  unsigned dx,
                                  unsigned dy)
{
    walk->dx = dx;
    walk->dy = dy;
}

unsigned zebra_img_walker_get_col (zebra_img_walker_t *walk)
{
    return(walk->x);
}

unsigned zebra_img_walker_get_row (zebra_img_walker_t *walk)
{
    return(walk->y);
}

typedef struct walk_state_s {
    unsigned pdx, pdy;
    unsigned x0, y0;
    int du, pdu;
    const void *p0;
} walk_state_t;

static char walk_x_diag (zebra_img_walker_t *walk,
                         walk_state_t *st)
{
    int err = walk->h - walk->w;
    const void *p = st->p0;
    walk->x = st->x0;
    walk->y = st->y0;
    while(walk->y < walk->h) {
        if(!walk->x || (walk->y == st->y0))
            dprintf("walk_x: %03x,%03x (%06lx)\n",
                    walk->x, walk->y, (unsigned long)p);
        if(walk->handler) {
            char rc;
            if((rc = walk->handler(walk, (void*)p)))
                return(rc);
        }
        if(err >= 0) {
            walk->y += st->du;
            p += st->pdu;
            err -= walk->w * 2;
        }
        err += walk->h;
        p += st->pdx;
        if(++walk->x >= walk->w) {
            walk->x = 0;
            p -= st->pdy;
        }
    }
    return(0);
}

static char walk_y_diag (zebra_img_walker_t *walk,
                         walk_state_t *st)
{
    int err = walk->w - walk->h;
    const void *p = st->p0;
    walk->x = st->x0;
    walk->y = st->y0;
    while(walk->y < walk->h) {
        if(!walk->x || (walk->y == st->y0))
            dprintf("walk_y: %03x,%03x (%06lx)\n",
                    walk->x, walk->y, (unsigned long)p);
        if(walk->handler) {
            char rc;
            if((rc = walk->handler(walk, (void*)p)))
                return(rc);
        }
        if(err >= 0) {
            if(++walk->x >= walk->w) {
                walk->x = 0;
                p -= st->pdy;
            }
            p += st->pdx;
            err -= walk->h * 2;
        }
        err += walk->w;
        p += st->pdu;
        walk->y += st->du;
    }
    return(0);
}

char zebra_img_walk (zebra_img_walker_t *walk,
                     const void *image)
{
    if(!walk->w || !walk->h)
        return(-1);

    char rc;
    walk_state_t st;
    st.pdx = (walk->dx) ? walk->dx : 1;
    unsigned dx_w = st.pdx * walk->w;
    st.pdy = (walk->dy >= dx_w) ? walk->dy : dx_w;

    /* FIXME sanitize parameters (no inf loops!) */
    dprintf("walk: w=%x h=%x dx=%x dy=%x x=%x y=%x\n",
            walk->w, walk->h, st.pdx, st.pdy, walk->x, walk->y);

    /* start w/a few passes along the major axis */
    unsigned di;
    uintptr_t dp;
    if(walk->w > walk->h) {
        di = walk->h / 8; /* density (FIXME config) */
        dp = di * st.pdy;
    }
    else {
        di = walk->w / 8;
        dp = di * st.pdx;
    }
    st.p0 = image + dp * 2;

    unsigned i;
    for(i = di * 2; i <= di * 6; i += di, st.p0 += dp) {
        const void *p = st.p0;
        if(walk->w > walk->h) {
            walk->y = i;
            for(walk->x = 0; walk->x < walk->w; walk->x++, p += st.pdx)
                if(walk->handler && ((rc = walk->handler(walk, (void*)p))))
                    return(rc);
        }
        else {
            walk->x = i;
            for(walk->y = 0; walk->y < walk->h; walk->y++, p += st.pdy)
                if(walk->handler && ((rc = walk->handler(walk, (void*)p))))
                    return(rc);
        }
    }
    

    /* then resort to grid */
    di = walk->w / 4 /* density (FIXME config) */;
    dp = di * st.pdx;

    st.x0 = st.y0 = 0;
    st.p0 = image;
    st.du = 1;
    st.pdu = st.pdy;
    for(st.x0 = 0; st.x0 < walk->w; st.x0 += di, st.p0 += dp)
        if((rc = walk_x_diag(walk, &st)))
            return(rc);

    st.y0 = walk->h - 1;
    st.p0 = image + st.pdy * st.y0;
    st.pdu = -st.pdy;
    st.du = -1;
    for(st.x0 = 0; st.x0 < walk->w; st.x0 += di, st.p0 += dp)
        if((rc = walk_x_diag(walk, &st)))
            return(rc);

    di /= 2;
    dp /= 2;
    st.p0 = image + st.pdy * st.y0;
    for(st.x0 = 0; st.x0 < walk->w; st.x0 += di, st.p0 += dp)
        if((rc = walk_y_diag(walk, &st)))
            return(rc);

    st.y0 = 0;
    st.p0 = image;
    st.du = 1;
    st.pdu = st.pdy;
    for(st.x0 = 0; st.x0 < walk->w; st.x0 += di, st.p0 += dp)
        if((rc = walk_y_diag(walk, &st)))
            return(rc);

    return(0);
}
