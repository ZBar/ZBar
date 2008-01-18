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

#include "window.h"
#include "image.h"

zebra_window_t *zebra_window_create ()
{
    zebra_window_t *w = calloc(1, sizeof(zebra_window_t));
    if(!w)
        return(NULL);
    err_init(&w->err, ZEBRA_MOD_WINDOW);
    w->overlay = 1;
    return(w);
}

void zebra_window_destroy (zebra_window_t *w)
{
    /* detach */
    zebra_window_attach(w, NULL, 0);
    err_cleanup(&w->err);
    free(w);
}

int zebra_window_attach (zebra_window_t *w,
                         void *display,
                         unsigned long drawable)
{
#if !defined(HAVE_X)
    err_capture(vdo, SEV_ERROR, ZEBRA_ERR_UNSUPPORTED, __func__,
                "not compiled with output window support");
    _zebra_spew_error(vdo);
    return(-1);
#else
    /* release image */
    zebra_window_draw(w, NULL);
    if(w->cleanup) {
        w->cleanup(w);
        w->cleanup = NULL;
        w->draw_image = NULL;
    }
    if(w->formats) {
        free(w->formats);
        w->formats = NULL;
    }
    return(_zebra_window_attach(w, display, drawable));
#endif
}

static inline int window_draw_overlay (zebra_window_t *w)
{
    /* FIXME TBD */
    /* _zebra_draw_marker, _zebra_draw_line,
     * _zebra_draw_polygon, _zebra_draw_text, etc...
     */
    if(!w->overlay)
        return(0);
    if(w->overlay >= 1) {
        /* outline each symbol */
    }
    if(w->overlay >= 2) {
        /* calculate/display frame rate */
    }
    return(0);
}

inline int zebra_window_redraw (zebra_window_t *w)
{
    if(!w->image || !w->draw_image)
        return(_zebra_window_clear(w));
    int rc = w->draw_image(w, w->image);
    if(rc)
        return(rc);
    return(window_draw_overlay(w));
}

int zebra_window_draw (zebra_window_t *w,
                       zebra_image_t *img)
{
    if(!w->draw_image)
        img = NULL;
    if(w->image)
        _zebra_image_refcnt(w->image, -1);
    w->image = img;
    if(img)
        img->refcnt++;
    return(zebra_window_redraw(w));
}

void zebra_window_set_overlay (zebra_window_t *w,
                               int lvl)
{
    if(lvl < 0)
        lvl = 0;
    if(lvl > 2)
        lvl = 2;
    if(w->overlay != lvl) {
        w->overlay = lvl;
        zebra_window_redraw(w);
    }
}

int zebra_window_resize (zebra_window_t *w,
                         unsigned width,
                         unsigned height)
{
    w->width = width;
    w->height = height;
    return(0);
}
