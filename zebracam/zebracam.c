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

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/types.h>
#include <linux/videodev.h>
#include <SDL/SDL.h>

#include "zebra.h"

static int cam_fd;
static struct video_capability vcap;
static struct video_window vwin;
static uint32_t size;
struct video_picture vpic;
struct video_mbuf vbuf;
struct video_mmap vmap;
void *image;
SDL_Surface *screen;
uint8_t *buf;
int streaming = 1, dump = 0;

zebra_decoder_t *decoder;
zebra_scanner_t *scanner;

static void init_cam ()
{
    cam_fd = open("/dev/video0", O_RDWR);
    if(cam_fd < 0) {
        perror("ERROR opening /dev/video0\n");
        exit(1);
    }

    if(ioctl(cam_fd, VIDIOCGCAP, &vcap) < 0) {
        perror("ERROR VIDIOCGCAP: v4l unsupported\n");
        exit(1);
    }
    printf("found device: \"%s\"\n", vcap.name);


    /* initialize window properties */
    if(ioctl(cam_fd, VIDIOCGWIN, &vwin) < 0) {
        perror("ERROR VIDIOCGWIN");
        exit(1);
    }

    if(vwin.width != vcap.maxwidth) {
        vwin.width = vcap.maxwidth;
        vwin.height = vcap.maxheight;

        if(ioctl(cam_fd, VIDIOCSWIN, &vwin) < 0) {
            perror("ERROR VIDIOCSWIN");
            exit(1);
        }
        if(ioctl(cam_fd, VIDIOCGWIN, &vwin) < 0) {
            perror("ERROR VIDIOCGWIN");
            exit(1);
        }
    }
    size = vwin.width * vwin.height;
    printf("    win: %dx%d @(%d, %d) = 0x%x bytes\n",
           vwin.width, vwin.height, vwin.x, vwin.y, size);


    /* setup image properties */
    if(ioctl(cam_fd, VIDIOCGPICT, &vpic) < 0) {
        perror("ERROR VIDIOCGPICT");
        exit(1);
    }

    int palette = VIDEO_PALETTE_RGB24;
    if(vpic.palette != palette) {
        vpic.palette = palette;
        if(ioctl(cam_fd, VIDIOCSPICT, &vpic) < 0) {
            perror("ERROR VIDIOCSPICT");
            exit(1);
        }
        if(ioctl(cam_fd, VIDIOCGPICT, &vpic) < 0) {
            perror("ERROR VIDIOCGPICT");
            exit(1);
        }
        if(vpic.palette != palette) {
            fprintf(stderr, "ERROR can't set palette\n");
            exit(1);
        }
    }
    printf("    pict: brite=%04x hue=%04x color=%04x con=%04x"
           " wht=%04x deep=%04x pal=%04x\n",
           vpic.brightness, vpic.hue, vpic.colour, vpic.contrast,
           vpic.whiteness, vpic.depth, vpic.palette);


    /* map camera image to memory */
    if(ioctl(cam_fd, VIDIOCGMBUF, &vbuf) < 0) {
        perror("ERROR VIDIOCGMBUF");
        exit(1);
    }
    printf("    buf: size=%x num=%x [0]=%x [1]=%x\n",
           vbuf.size, vbuf.frames, vbuf.offsets[0], vbuf.offsets[1]);

    /* only need double buffer */
    if(vbuf.frames > 2)
        vbuf.frames = 2;

    image = mmap(0, vbuf.size, PROT_READ | PROT_WRITE, MAP_SHARED, cam_fd, 0);

    /* initialize for capture */
    vmap.frame = 0;
    vmap.height = vwin.height;
    vmap.width = vwin.width;
    vmap.format = vpic.palette;
}

static void init_sdl ()
{
    /* initialize SDL */
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "ERROR initializing SDL: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);
    SDL_WM_SetCaption("zebracam", "zebracam");

    setenv("SDL_VIDEO_CENTERED", "1", 1);  
    screen = SDL_SetVideoMode(vwin.width, vwin.height,
                              24, SDL_SWSURFACE | SDL_DOUBLEBUF);
    if(!screen) {
        fprintf(stderr, "ERROR setting video mode: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_ShowCursor(1);
}

static inline int handle_events ()
{
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT ||
           (e.type == SDL_KEYDOWN &&
            e.key.keysym.sym == SDLK_q))
            /* exit condition */
            return(1);
        if(e.type == SDL_KEYDOWN &&
           e.key.keysym.sym == SDLK_SPACE) {
            /* pause/resume */
            streaming = !streaming;
            dump = !streaming;
        }
    }
    return(0);
}

static inline void sync (int frame)
{
    if(ioctl(cam_fd, VIDIOCSYNC, &frame) < 0) {
        perror("ERROR VIDIOCSYNC");
        exit(1);
    }
}

static inline void capture (int frame)
{
    vmap.frame = frame;
    if(ioctl(cam_fd, VIDIOCMCAPTURE, &vmap) < 0) {
        perror("ERROR VIDIOCMCAPTURE");
        exit(1);
    }
}

static inline void display ()
{
    memcpy(screen->pixels, buf, size * 3);
}

static void process ()
{
    uint32_t dy = vwin.width * 3;
    uint8_t *p = buf + ((vwin.height + 1) >> 1) * dy;
    int i;
    for(i = 0; i < vwin.width; i++, p += 3) {
        if(zebra_scan_rgb24(scanner, p) > 1)
            printf("%x: %s\n",
                   zebra_decoder_get_type(decoder),
                   zebra_decoder_get_data(decoder));
        *(p + 2) = 0xff;
    }
}


int main (int argc, const char *argv[])
{
    /* initialization */
    init_cam();
    init_sdl();
    decoder = zebra_decoder_create();
    scanner = zebra_scanner_create(decoder);

    /* main loop */
    int frame = 0;
    capture(frame);
    capture(1);

    int nframes = 0;

    while(!handle_events()) {
        /* wait for latest image */
        sync(frame);
        buf = image + vbuf.offsets[frame];

        if(streaming) {
            if(SDL_MUSTLOCK(screen) &&
               SDL_LockSurface (screen) < 0) {
                fprintf(stderr, "ERROR SDL failed to lock screen surface: %s\n",
                        SDL_GetError());
                return(1);
            }

            /* application processing */
            process();

            /* show result onscreen */
            display();

            if(SDL_MUSTLOCK(screen))
                SDL_UnlockSurface(screen);
        
            SDL_Flip(screen);
        }
        else if(dump) {
            process();
            dump = 0;
        }

        /* re-start capture of image */
        capture(frame);

        nframes++;
        if(!(nframes & 0xf)) {
            double ms = SDL_GetTicks();
            double fr = nframes * 1000. / ms;
            printf("@%g %gfps\n", ms, fr);
        }
        frame = (frame + 1) % vbuf.frames;
    }

    zebra_scanner_destroy(scanner);
    zebra_decoder_destroy(decoder);
    return(0);
}
