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
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>

#include <zbar.h>
#include "processor.h"
#include "test_images.h"

zbar_processor_t proc;

static void input_wait ()
{
    fprintf(stderr, "waiting for input...\n");
    if(_zbar_window_handle_events(&proc, 1) < 0)
        zbar_processor_error_spew(&proc, 1);
}

int main (int argc, char *argv[])
{
    zbar_set_verbosity(32);

    err_init(&proc.err, ZBAR_MOD_PROCESSOR);
    proc.window = zbar_window_create();
    if(!proc.window) {
        fprintf(stderr, "unable to allocate memory?!\n");
        return(1);
    }

    int width = 640;
    int height = 480;

    if(_zbar_window_open(&proc, "zbar window test", width, height) ||
       zbar_window_attach(proc.window, proc.display, proc.xwin) ||
       _zbar_window_set_visible(&proc, 1)) {
        fprintf(stderr, "failed to open test window\n");
        return(1);
    }
    input_wait();

    zbar_image_t *img = zbar_image_create();
    zbar_image_set_size(img, width, height);
    zbar_image_set_format(img, fourcc('B','G','R','4'));
    /*fourcc('I','4','2','0')*/
    /*fourcc('Y','V','1','2')*/
    /*fourcc('U','Y','V','Y')*/
    /*fourcc('Y','U','Y','V')*/
    /*fourcc('R','G','B','3')*/
    /*fourcc('Y','8','0','0')*/
    test_image_bars(img);

    if(zbar_window_draw(proc.window, img) ||
       zbar_window_redraw(proc.window)) {
        fprintf(stderr, "error drawing image\n");
        return(1);
    }
    zbar_image_destroy(img);
    img = NULL;

    input_wait();

    /* FIXME display cmd arg images? or formats? */

    fprintf(stderr, "cleaning up\n");
    zbar_window_destroy(proc.window);

    /* FIXME destructor check? */
    if(test_image_check_cleanup())
        return(32);
    return(0);
}
