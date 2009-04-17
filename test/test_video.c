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
#include <time.h>
#include <assert.h>

#include <zbar.h>
#include "processor.h"          /* reuse some processor guts */
#include "test_images.h"

zbar_processor_t proc;

int main (int argc, char *argv[])
{
    zbar_set_verbosity(10);

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
    proc.video = zbar_video_create();
    proc.window = zbar_window_create();
    if(!proc.video || !proc.window) {
        fprintf(stderr, "unable to allocate memory?!\n");
        return(1);
    }

    if(zbar_video_open(proc.video, dev)) {
        zbar_video_error_spew(proc.video, 0);
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
           dev, zbar_video_get_fd(proc.video));

    if(_zbar_window_open(&proc, "zbar video test", 640, 480) ||
       zbar_window_attach(proc.window, proc.display, proc.xwin) ||
       _zbar_window_set_visible(&proc, 1))
        fprintf(stderr, "WARNING: failed to open test window\n");
    else if(zbar_negotiate_format(proc.video, proc.window))
        fprintf(stderr, "WARNING: failed to negotiate compatible format\n");

    zbar_image_t *img = zbar_image_create();
    zbar_image_set_size(img, 640, 480);
    zbar_image_set_format(img, fourcc('Y','V','1','2'));
    test_image_bars(img);
    if(proc.display && zbar_window_draw(proc.window, img)) {
        fprintf(stderr, "ERROR: drawing image\n");
        return(zbar_window_error_spew(proc.window, 0));
    }

    if(zbar_video_enable(proc.video, 1)) {
        fprintf(stderr, "ERROR: starting video stream\n");
        return(zbar_video_error_spew(proc.video, 0));
    }

    zbar_image_t *image = zbar_video_next_image(proc.video);
    if(!image) {
        fprintf(stderr, "ERROR: unable to capture image\n");
        return(zbar_video_error_spew(proc.video, 0));
    }
    uint32_t format = zbar_image_get_format(image);
    unsigned width = zbar_image_get_width(image);
    unsigned height = zbar_image_get_height(image);
    const uint8_t *data = zbar_image_get_data(image);
    printf("captured image: %d x %d %.4s @%p\n",
           width, height, (char*)&format, data);

    if(proc.display && zbar_window_draw(proc.window, image)) {
        fprintf(stderr, "ERROR: drawing image\n");
        return(zbar_window_error_spew(proc.window, 0));
    }
    zbar_image_destroy(image);

    printf("\nstreaming...click to continue\n");
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);
    int n = 0;
    while(1) {
        if(proc.display && _zbar_window_handle_events(&proc, 0))
            break;
        zbar_image_t *image = zbar_video_next_image(proc.video);
        if(!image) {
            fprintf(stderr, "ERROR: unable to capture image\n");
            return(zbar_video_error_spew(proc.video, 0));
        }

        if(proc.display && zbar_window_draw(proc.window, image)) {
            fprintf(stderr, "ERROR: drawing image\n");
            return(zbar_window_error_spew(proc.window, 0));
        }
        zbar_image_destroy(image);
        n++;
    }
    clock_gettime(CLOCK_REALTIME, &end);
    double ms = (end.tv_sec - start.tv_sec +
                 (end.tv_nsec - start.tv_nsec) / 1000000000.);
    double fps = n / ms;
    printf("\nprocessed %d images in %gs @%gfps\n", n, ms, fps);

    if(zbar_video_enable(proc.video, 0)) {
        fprintf(stderr, "ERROR: while stopping video stream\n");
        return(zbar_video_error_spew(proc.video, 0));
    }

    printf("\nvideo support verified\n\n");

    if(proc.display && zbar_window_draw(proc.window, img)) {
        fprintf(stderr, "error drawing image\n");
        return(zbar_window_error_spew(proc.window, 0));
    }
    zbar_image_destroy(img);
    printf("click to end...\n");
    if(proc.display)
        _zbar_window_handle_events(&proc, 1);

    zbar_window_destroy(proc.window);
    zbar_video_destroy(proc.video);

    if(test_image_check_cleanup())
        return(32);
    return(0);
}
