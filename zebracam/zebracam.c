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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <assert.h>
#include <linux/types.h>
#include <linux/videodev.h>
#include <SDL/SDL.h>

#include <zebra.h>

#define DUMP_NAME_MAX 0x20
#define PPM_HDR_MAX 0x20
#define SYM_MAX 0x20
#define SYM_HYST 2000 /* ms */

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
int streaming = 1, dumping = 0;
uint64_t nframes = 0;
double last_frame_time = 0;

SDL_AudioSpec audio;
#define TONE_MAX  4096
static char tone[TONE_MAX];
int tone_period = 20;

zebra_symbol_type_t last_sym_type;
char last_sym_data[SYM_MAX];
double last_sym_time = 0;

double beep_time = 0;

zebra_decoder_t *decoder;
zebra_scanner_t *scanner;
zebra_img_walker_t *walker;

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
    fprintf(stderr, "found device: \"%s\"\n", vcap.name);


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
    fprintf(stderr, "    win: %dx%d @(%d, %d) = 0x%x bytes\n",
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
    fprintf(stderr, "    pict: brite=%04x hue=%04x color=%04x con=%04x"
            " wht=%04x deep=%04x pal=%04x\n",
            vpic.brightness, vpic.hue, vpic.colour, vpic.contrast,
            vpic.whiteness, vpic.depth, vpic.palette);


    /* map camera image to memory */
    if(ioctl(cam_fd, VIDIOCGMBUF, &vbuf) < 0) {
        perror("ERROR VIDIOCGMBUF");
        exit(1);
    }
    fprintf(stderr, "    buf: size=%x num=%x [0]=%x [1]=%x\n",
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

static void audio_cb (void *userdata,
                      uint8_t *stream,
                      int len)
{
    memcpy(stream, tone, len);
    SDL_PauseAudio(1);
}

static void init_sdl ()
{
    /* initialize SDL */
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
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

    SDL_AudioSpec audio;
    audio.freq = 11025;
    audio.format = AUDIO_S8;
    audio.channels = 1;
    audio.samples = TONE_MAX;
    audio.callback = audio_cb;
    audio.userdata = NULL;
    if(SDL_OpenAudio(&audio, NULL) < 0) {
        fprintf(stderr, "ERROR initializing SDL audio: %s\n", SDL_GetError());
        exit(1);
    }
    fprintf(stderr, "audio: size=%04x silence=%02x\n",
            audio.size, audio.silence);
    int i;
    for(i = 0; (i % tone_period) || (i + tone_period < TONE_MAX); i++)
        tone[i] = ((i % tone_period) > (tone_period + 1) / 2) ? 0x7f : -0x7f;
    for(; i < TONE_MAX; i++)
        tone[i] = audio.silence;
}

static inline int handle_events ()
{
    SDL_Event e;
    while(SDL_PollEvent(&e)) {
        if(e.type == SDL_QUIT)
            return(1);
        if(e.type == SDL_KEYDOWN) {
            switch(e.key.keysym.sym) {
            case SDLK_q:
                return(1);
            case SDLK_SPACE:
                streaming = !streaming; /* pause/resume */
                break;
            case SDLK_d:
                dumping = 1; /* dump next frame */
                break;
            default:
                /* ignore */;
            }
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

static inline void dump ()
{
    char name[DUMP_NAME_MAX];
    int len = snprintf(name, DUMP_NAME_MAX, "zebracam-%08llx.ppm", nframes);
    assert(len < DUMP_NAME_MAX);
    FILE *f = fopen(name, "w");
    if(!f) {
        perror("ERROR opening dump file");
        return;
    }
    char hdr[PPM_HDR_MAX];
    len = snprintf(hdr, PPM_HDR_MAX, "P6\n%d %d\n255\n",
                   vwin.width, vwin.height);
    assert(len < PPM_HDR_MAX);
    if(fwrite(hdr, len, 1, f) != 1 ||
       /* FIXME RB planes swapped... (SDL/v4l is BGR, PPM is RGB) */
       fwrite(buf, 3, size, f) != size)
        perror("ERROR dumping to file");
    fclose(f);
    fprintf(stderr, "dumped frame to %s\n", name);
    dumping = 0;
}

static inline void process ()
{
    zebra_scanner_new_scan(scanner);
    zebra_img_walk(walker, buf);
}

static void symbol_handler (zebra_decoder_t *decoder)
{
    zebra_symbol_type_t sym = zebra_decoder_get_type(decoder);
    if(sym <= ZEBRA_PARTIAL)
        return;
    const char *data = zebra_decoder_get_data(decoder);

    double ms = SDL_GetTicks();
    /* edge detect symbol change w/some hysteresis */
    if(sym == last_sym_type &&
       !strncmp(data, last_sym_data, SYM_MAX) &&
       ((ms - last_sym_time) < SYM_HYST)) {
        last_sym_time = ms;
        return;
    }

    /* report new symbol */
    last_sym_time = ms;
    last_sym_type = sym;
    strncpy(last_sym_data, data, SYM_MAX);
    printf("%s%s: %s\n",
           zebra_get_symbol_name(sym),
           zebra_get_addon_name(sym),
           zebra_decoder_get_data(decoder));
    beep_time = ms;
    SDL_PauseAudio(0);
}

static char pixel_handler (zebra_img_walker_t *walker,
                           void *p)
{
    if(zebra_scan_rgb24(scanner, p) > ZEBRA_PARTIAL)
        return(1);
    *((uint8_t*)p - buf + (uint8_t*)screen->pixels + 2) = 0xff;
    return(0);
}

int usage (int rc)
{
    FILE *out = (rc) ? stderr : stdout;
    fprintf(out,
            "usage: zebracam [-h|--help|--version]\n"
            "\n"
            "scan and decode bar codes from a video stream\n"
            "\n"
            "options:\n"
            "    -h, --help     display this help text\n"
            "    --version      display version information and exit\n"
            "\n");
    return(rc);
}

int main (int argc, const char *argv[])
{
    int i;
    for(i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-h") ||
           !strcmp(argv[i], "--help"))
            return(usage(0));
        if(!strcmp(argv[i], "--version"))
            return(printf(PACKAGE_VERSION "\n") <= 0);
        else
            fprintf(stderr, "WARNING: ignoring unknown argument: %s\n",
                    argv[i]);
    }

    /* initialization */
    init_cam();
    init_sdl();

    /* create decoder object */
    decoder = zebra_decoder_create();
    /* attach callback */
    zebra_decoder_set_handler(decoder, symbol_handler);

    /* attach new scanner to decoder */
    scanner = zebra_scanner_create(decoder);

    /* setup image scan pattern walker */
    walker = zebra_img_walker_create();
    zebra_img_walker_set_size(walker, vwin.width, vwin.height);
    zebra_img_walker_set_stride(walker, 3, 0); /* RGB */
    zebra_img_walker_set_handler(walker, pixel_handler);

    /* main loop */
    int frame = 0;
    capture(frame);
    capture(1);

    while(!handle_events()) {
        /* wait for latest image */
        sync(frame);
        buf = image + vbuf.offsets[frame];

        if(streaming) {
            if(SDL_MUSTLOCK(screen) &&
               SDL_LockSurface (screen) < 0) {
                fprintf(stderr,
                        "ERROR SDL failed to lock screen surface: %s\n",
                        SDL_GetError());
                return(1);
            }

            /* show result onscreen */
            display();

            /* application processing */
            process();

            /* dump frame to file */
            if(dumping)
                dump();

            if(SDL_MUSTLOCK(screen))
                SDL_UnlockSurface(screen);

            SDL_Flip(screen);
        }
        else if(dumping)
            dump();

        /* re-start capture of image */
        capture(frame);

        nframes++;
        double ms = SDL_GetTicks();
        if(ms >= last_frame_time + 10000) {
            last_frame_time += 10000;
            double fr = nframes * 1000. / ms;
            fprintf(stderr, "@%g %gfps\n", ms, fr);
        }
        frame = (frame + 1) % vbuf.frames;
    }

    zebra_scanner_destroy(scanner);
    zebra_decoder_destroy(decoder);
    return(0);
}
