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
#include <string.h>
#include <time.h>
#include <assert.h>

#include <zebra.h>
#include "processor.h"          /* reuse some processor guts */
#include "test_images.h"

zebra_processor_t proc;

int main (int argc, char *argv[])
{
    zebra_set_verbosity(10);

    const char *dev = "/dev/video0";
    if(argc > 1) {
        if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
            printf("usage: %s [/dev/[v4l/]videoX]\n", argv[0]);
            return(0);
        }
        else if(!strcmp(argv[1], "--version")) {
            printf(PACKAGE_VERSION "\n");
            return(0);
        }
        dev = argv[1];
    }

    memset(&proc, 0, sizeof(proc));
    proc.video = zebra_video_create();
    proc.window = zebra_window_create();
    if(!proc.video || !proc.window) {
        fprintf(stderr, "unable to allocate memory?!\n");
        return(1);
    }

    if(zebra_video_open(proc.video, dev)) {
        zebra_video_error_spew(proc.video, 0);
        fprintf(stderr,
                "ERROR: unable to access your video device\n"
                "this program requires video capture support using"
                " v4l version 1 or 2\n"
                "    - is your video device located at \"%s\"?\n"
                "    - is your video driver installed? (check dmesg)\n"
                "    - make sure you have the latest drivers\n"
                "    - do you have read/write permission to access it?\n"
                "    - is another application is using it?\n"
                "    - does the device support video capture?\n"
                "    - does the device work with other programs?\n",
                dev);
        return(1);
    }
    printf("opened video device: %s (fd=%d)\n",
           dev, zebra_video_get_fd(proc.video));

    if(_zebra_window_open(&proc, "zebra video test", 640, 480) ||
       zebra_window_attach(proc.window, proc.display, proc.xwin) ||
       _zebra_window_set_visible(&proc, 1))
        fprintf(stderr, "WARNING: failed to open test window\n");
    else if(zebra_negotiate_format(proc.video, proc.window))
        fprintf(stderr, "WARNING: failed to negotiate compatible format\n");

    zebra_image_t *img = zebra_image_create();
    zebra_image_set_size(img, 640, 480);
    zebra_image_set_format(img, fourcc('Y','V','1','2'));
    test_image_bars(img);
    if(proc.display && zebra_window_draw(proc.window, img)) {
        fprintf(stderr, "ERROR: drawing image\n");
        return(zebra_window_error_spew(proc.window, 0));
    }

    if(zebra_video_enable(proc.video, 1)) {
        fprintf(stderr, "ERROR: starting video stream\n");
        return(zebra_video_error_spew(proc.video, 0));
    }

    zebra_image_t *image = zebra_video_next_image(proc.video);
    if(!image) {
        fprintf(stderr, "ERROR: unable to capture image\n");
        return(zebra_video_error_spew(proc.video, 0));
    }
    uint32_t format = zebra_image_get_format(image);
    unsigned width = zebra_image_get_width(image);
    unsigned height = zebra_image_get_height(image);
    const uint8_t *data = zebra_image_get_data(image);
    printf("captured image: %d x %d %.4s @%p\n",
           width, height, (char*)&format, data);

    if(proc.display && zebra_window_draw(proc.window, image)) {
        fprintf(stderr, "ERROR: drawing image\n");
        return(zebra_window_error_spew(proc.window, 0));
    }
    zebra_image_destroy(image);

    printf("\nstreaming...click to continue\n");
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    int n = 0;
    while(1) {
        if(proc.display && _zebra_window_handle_events(&proc, 0))
            break;
        zebra_image_t *image = zebra_video_next_image(proc.video);
        if(!image) {
            fprintf(stderr, "ERROR: unable to capture image\n");
            return(zebra_video_error_spew(proc.video, 0));
        }

        if(proc.display && zebra_window_draw(proc.window, image)) {
            fprintf(stderr, "ERROR: drawing image\n");
            return(zebra_window_error_spew(proc.window, 0));
        }
        zebra_image_destroy(image);
        n++;
    }
    clock_gettime(CLOCK_REALTIME, &end);
    double ms = (end.tv_sec - start.tv_sec +
                 (end.tv_nsec - start.tv_nsec) / 1000000000.);
    double fps = n / ms;
    printf("\nprocessed %d images in %gs @%gfps\n", n, ms, fps);

    if(zebra_video_enable(proc.video, 0)) {
        fprintf(stderr, "ERROR: while stopping video stream\n");
        return(zebra_video_error_spew(proc.video, 0));
    }

    printf("\nvideo support verified\n\n");

    if(proc.display && zebra_window_draw(proc.window, img)) {
        fprintf(stderr, "error drawing image\n");
        return(zebra_window_error_spew(proc.window, 0));
    }
    zebra_image_destroy(img);
    printf("click to end...\n");
    if(proc.display)
        _zebra_window_handle_events(&proc, 1);

    zebra_window_destroy(proc.window);
    zebra_video_destroy(proc.video);

    if(test_image_check_cleanup())
        return(32);
    return(0);
}
