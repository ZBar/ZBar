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

#include <config.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include <zebra.h>
#include "test_images.h"

zebra_processor_t *proc = NULL;

int input_wait (int timeout)
{
    if(timeout >= 0)
        printf("waiting %d.%03ds for input...",
               timeout / 1000, timeout % 1000);
    else
        printf("waiting indefinitely for input...");
    fflush(stdout);
    int rc = zebra_processor_user_wait(proc, timeout);
    if(rc > 0)
        printf("got input\n");
    else if(!rc ||
            (rc < 0 &&
             zebra_processor_get_error_code(proc) == ZEBRA_ERR_CLOSED)) {
        printf("timed out\n");
        return(0);
    }
    return(rc);
}

int main (int argc, char **argv)
{
    zebra_set_verbosity(31);
    char *video_dev = NULL;
    int i, use_threads = 0, use_window = 0;
    for(i = 1; i < argc; i++) {
        if(argv[i][0] == '-') {
            if(!strncmp(argv[i], "-thr", 4))
                use_threads = 1;
            else if(!strncmp(argv[i], "-win", 4))
                use_window = 1;
            else
                return(1);
        }
        else
            video_dev = argv[i];
    }

    proc = zebra_processor_create(use_threads);
    assert(proc);
    printf("created processor (%sthreaded)\n", (!use_threads) ? "un" : "");

    if(zebra_processor_init(proc, NULL, 0))
        return(2);
    printf("initialized (video=disabled, window=disabled)\n");

    zebra_image_t *img = zebra_image_create();
    zebra_image_set_size(img, 640, 480);
    zebra_image_set_format(img, fourcc('Y','V','1','2'));
    test_image_bars(img);

    if(zebra_process_image(proc, img) < 0)
        return(3);
    printf("processed test image\n");

    if(zebra_processor_init(proc, video_dev, use_window))
        return(2);
    printf("reinitialized (video=%s, window=%s)\n",
           (video_dev) ? video_dev : "disabled",
           (use_window) ? "enabled" : "disabled");

    if(zebra_process_image(proc, img) < 0)
        return(3);
    printf("processed test image\n");
    zebra_image_destroy(img);

    if(use_window) {
        if(zebra_processor_set_visible(proc, 1))
            return(4);
        printf("window visible\n");
    }

    if(input_wait((use_window && !video_dev) ? -1 : 3333) < 0)
        return(5);

    if(video_dev) {
        if(zebra_processor_set_active(proc, 1))
            return(3);
        printf("video activated\n");

        if(input_wait((use_window) ? -1 : 4000) < 0)
            return(5);

        if(zebra_processor_set_active(proc, 0))
            return(3);
        printf("video deactivated\n");

        if(input_wait((use_window) ? -1 : 4000) < 0)
            return(5);

        /* FIXME test process_one() */
    }

    if(zebra_process_image(proc, NULL))
        return(3);
    printf("flushed image\n");

    if(input_wait((use_window && !video_dev) ? -1 : 2500) < 0)
        return(5);

    zebra_processor_destroy(proc);
    proc = NULL;
    if(test_image_check_cleanup())
        return(32);
    return(0);
}
