/*------------------------------------------------------------------------
 *  Copyright 2008 (c) Jeff Brown <spadix@users.sourceforge.net>
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

static inline int null_error (void *m,
                              const char *func)
{
    return(err_capture(m, SEV_ERROR, ZEBRA_ERR_UNSUPPORTED, func,
                       "not compiled with output window support"));
}

int _zebra_window_attach (zebra_window_t *w,
                          void *display,
                          unsigned long win)
{
    return(null_error(w, __func__));
}

int _zebra_window_handle_events (zebra_processor_t *proc,
                                 int block)
{
    return(null_error(proc, __func__));
}

int _zebra_window_open (zebra_processor_t *proc,
                        char *title,
                        unsigned width,
                        unsigned height)
{
    return(null_error(proc, __func__));
}

int _zebra_window_close (zebra_processor_t *proc)
{
    return(null_error(proc, __func__));
}

int _zebra_window_resize (zebra_processor_t *proc,
                          unsigned width,
                          unsigned height)
{
    return(null_error(proc, __func__));
}

int _zebra_window_set_visible (zebra_processor_t *proc,
                               int visible)
{
    return(null_error(proc, __func__));
}

int _zebra_window_clear (zebra_window_t *w)
{
    return(null_error(w, __func__));
}

int _zebra_window_draw_marker(zebra_window_t *w,
                              uint32_t rgb,
                              const point_t *p)
{
    return(null_error(w, __func__));
}
