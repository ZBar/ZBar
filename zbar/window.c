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

#include "window.h"
#include "image.h"

extern int _zbar_window_draw_logo(zbar_window_t *w);

zbar_window_t *zbar_window_create ()
{
    zbar_window_t *w = calloc(1, sizeof(zbar_window_t));
    if(!w)
        return(NULL);
    err_init(&w->err, ZBAR_MOD_WINDOW);
    w->overlay = 1;
    return(w);
}

void zbar_window_destroy (zbar_window_t *w)
{
    /* detach */
    zbar_window_attach(w, NULL, 0);
    err_cleanup(&w->err);
    free(w);
}

int zbar_window_attach (zbar_window_t *w,
                        void *display,
                        unsigned long drawable)
{
    /* release image */
    zbar_window_draw(w, NULL);
    if(w->cleanup) {
        w->cleanup(w);
        w->cleanup = NULL;
        w->draw_image = NULL;
    }
    if(w->formats) {
        free(w->formats);
        w->formats = NULL;
    }
    return(_zbar_window_attach(w, display, drawable));
}

static inline int window_draw_overlay (zbar_window_t *w)
{
    /* FIXME TBD
     * _zbar_draw_line, _zbar_draw_polygon, _zbar_draw_text, etc...
     */
    if(!w->overlay)
        return(0);
    if(w->overlay >= 1 && w->image) {
        /* FIXME outline each symbol */
        const zbar_symbol_t *sym = zbar_image_first_symbol(w->image);
        for(; sym; sym = sym->next) {
            int i;
            for(i = 0; i < sym->npts; i++) {
                uint32_t color = ((sym->cache_count < 0) ? 4 : 2);
                _zbar_window_draw_marker(w, color, &sym->pts[i]);
            }
        }
    }
    if(w->overlay >= 2) {
        /* calculate/display frame rate */
    }
    return(0);
}

inline int zbar_window_redraw (zbar_window_t *w)
{
    if(window_lock(w))
        return(-1);
    if(!w->draw_image) {
        (void)window_unlock(w);
        return(_zbar_window_clear(w));
    }
    int rc = 0;
    if(w->image)
        rc = w->draw_image(w, w->image);
    else
        _zbar_window_draw_logo(w);
    if(!rc)
        rc = window_draw_overlay(w);
    (void)window_unlock(w);
    return(rc);
}

int zbar_window_draw (zbar_window_t *w,
                      zbar_image_t *img)
{
    if(window_lock(w))
        return(-1);
    if(!w->draw_image)
        img = NULL;
    if(w->image)
        _zbar_image_refcnt(w->image, -1);
    w->image = img;
    if(img)
        img->refcnt++;
    return(window_unlock(w));
}

void zbar_window_set_overlay (zbar_window_t *w,
                              int lvl)
{
    if(lvl < 0)
        lvl = 0;
    if(lvl > 2)
        lvl = 2;
    if(window_lock(w))
        return;
    if(w->overlay != lvl)
        w->overlay = lvl;
    (void)window_unlock(w);
}

int zbar_window_resize (zbar_window_t *w,
                        unsigned width,
                        unsigned height)
{
    if(window_lock(w))
        return(-1);
    w->width = width;
    w->height = height;
    _zbar_window_resize(w);
    return(window_unlock(w));
}
