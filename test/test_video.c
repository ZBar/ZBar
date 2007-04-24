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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <linux/types.h>
#include <linux/videodev.h>

static void dump_window (struct video_window *vwin)
{
    printf("    position  : %d x %d\n"
           "    size      : %d x %d\n"
           "    chromakey : 0x%x\n"
           "    flags     :",
           vwin->x, vwin->y,
           vwin->width, vwin->height,
           vwin->chromakey);
    static const char *vwin_flags[] = {
        "INTERLACE", "<unknown>", "<unknown>", "<unknown>", "CHROMAKEY",
    };
    int i;
    for(i = 0; i < 5; i++)
        if(vwin->flags & (1 << i))
            printf(" %s", vwin_flags[i]);
    printf("\n");
}

int main (int argc, char *argv[])
{
    char *dev = "/dev/video0";
    if(argc > 1) {
        if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
            printf("usage: %s [/dev/videoX]\n", argv[0]);
            return(0);
        }
        dev = argv[1];
    }

    int fd = open("/dev/video0", O_RDWR);
    if(fd < 0) {
        fprintf(stderr, "ERROR opening %s: ", dev);
        perror(NULL);
        fprintf(stderr,
                "    unable to access your video device:\n"
                "      - is video4linux installed?\n"
                "      - is your video driver installed?\n"
                "      - is your video device located at \"%s\"?\n"
                "      - do you have read/write permission to access it?\n"
                "      - does the device work with other programs?\n", dev);
        return(1);
    }
    printf("opened device %s\n", dev);

    /* check capabilities */
    struct video_capability vcap;
    if(ioctl(fd, VIDIOCGCAP, &vcap) < 0) {
        perror("ERROR VIDIOCGCAP");
        struct v4l2_capability vcap2;
        if(ioctl(fd, VIDIOC_QUERYCAP, &vcap2) < 0)
            fprintf(stderr, "    %s is not a v4l device\n", dev);
        else
            fprintf(stderr,
                    "    device only supports v4l version 2?\n"
                    "    this program requires v4l version 1 support\n"
                    "    (which should be available in emulation?)\n"
                    "      - make sure you have the latest drivers\n");
        return(1);
    }
    printf("%s supports v4l1\n"
           "    name      : %s\n"
           "    type      :",
           dev, vcap.name);
    static const char *types[] = {
        "CAPTURE", "TUNER", "TELETEXT", "OVERLAY",
        "CHROMAKEY", "CLIPPING", "FRAMERAM", "SCALES",
        "MONOCHROME", "SUBCAPTURE",
    };
    int i;
    for(i = 0; i < 10; i++)
        if(vcap.type & (1 << i))
            printf(" %s", types[i]);
    printf("\n"
           "    channels  : %d\n"
           "    audios    : %d\n"
           "    max size  : %d x %d\n"
           "    min size  : %d x %d\n"
           "\n",
           vcap.channels, vcap.audios,
           vcap.maxwidth, vcap.maxheight,
           vcap.minwidth, vcap.minheight);

    if(!(vcap.type & VID_TYPE_CAPTURE)) {
        fprintf(stderr,
                "ERROR: CAPTURE to memory not supported by device\n"
                "    this program requires the CAPTURE function\n"
                "      - make sure your device is a video capture device\n"
                "      - make sure the correct device is selected\n"
                "      - make sure you have the latest drivers\n");
        return(1);
    }

    /* initialize window properties */
    struct video_window vwin;
    if(ioctl(fd, VIDIOCGWIN, &vwin) < 0) {
        perror("ERROR VIDIOCGWIN");
        fprintf(stderr,
                "    unable to query window settings from driver\n"
                "    this is an unexpected driver communication error\n"
                "      - make sure you have the latest drivers\n");
        return(1);
    }

    printf("current window setting:\n");
    dump_window(&vwin);

    printf("attempting to set maximum size window...\n");
    vwin.width = vcap.maxwidth;
    vwin.height = vcap.maxheight;

    if(ioctl(fd, VIDIOCSWIN, &vwin) < 0) {
        perror("ERROR VIDIOCSWIN");
        fprintf(stderr,
                "    unable to update window settings\n"
                "    this is an unexpected driver communication error\n"
                "      - make sure you have the latest drivers\n"
                "(continuing anyway w/current window settings)\n");
    }
    if(ioctl(fd, VIDIOCGWIN, &vwin) < 0) {
        perror("ERROR VIDIOCGWIN");
        fprintf(stderr,
                "    unable to query window settings from driver\n"
                "    this is an unexpected driver communication error\n"
                "      - make sure you have the latest drivers\n"
                "(continuing anyway w/current window settings)\n");
    }
    printf("new window setting:\n");
    dump_window(&vwin);
    printf("\n");

    /* setup image properties */
    struct video_picture vpic;
    if(ioctl(fd, VIDIOCGPICT, &vpic) < 0) {
        perror("ERROR VIDIOCGPICT");
        fprintf(stderr,
                "    unable to query image properties from driver\n"
                "    this is an unexpected driver communication error\n");
        return(1);
    }
    static const char *palettes[] = {
        "<unknown>", "GREY", "HI240", "RGB565",
        "RGB24", "RGB32", "RGB555", "YUV422",
        "YUYV", "UYVY", "YUV420", "YUV411",
        "RAW", "YUV422P", "YUV411P", "YUV420P",
        "YUV410P",
    };
    printf("current image settings:\n"
           "    brightness: %d\n"
           "    hue       : %d\n"
           "    colour    : %d\n"
           "    contrast  : %d\n"
           "    whiteness : %d\n"
           "    depth     : %d\n"
           "    palette   : %s\n"
           "\n",
           vpic.brightness, vpic.hue, vpic.colour, vpic.contrast,
           vpic.whiteness, vpic.depth,
           ((vpic.palette <= VIDEO_PALETTE_YUV410P)
            ? palettes[vpic.palette] : "<unknown>"));

    printf("probing supported palettes:\n");
    uint32_t supported = 0;
    for(i = 1; i <= VIDEO_PALETTE_YUV410P; i++) {
        printf("    %-8s...", palettes[i]);
        fflush(stdout);
        vpic.palette = i;
        if(ioctl(fd, VIDIOCSPICT, &vpic) < 0) {
            printf("no (set fails)\n");
            continue;
        }
        if(ioctl(fd, VIDIOCGPICT, &vpic) < 0) {
            perror("\nERROR VIDIOCGPICT");
            fprintf(stderr,
                    "    unable to query image properties from driver\n"
                    "    this is an unexpected driver communication error\n");
            return(1);
        }
        if(vpic.palette != i) {
            printf("no (set ignored)\n");
            continue;
        }
        printf("yes\n");
        supported |= 1 << i;
    }
    printf("\n");

    if(!(supported & (1 << VIDEO_PALETTE_RGB24))) {
        fprintf(stderr,
                "ERROR: RGB24 palette not supported by device\n"
                "    this program currently requires RGB24 palette.\n"
                "    you can open a \"feature request\" from the sourceforge"
                " project page\n"
                "    to request support for additional palettes:\n"
                "        https://sourceforge.net/projects/zebra\n"
                "    please ensure you are running the latest drivers\n"
                "    and include the full output from this program\n");
        return(1);
    }

    /* reset to RGB24 */
    vpic.palette = VIDEO_PALETTE_RGB24;
    if(ioctl(fd, VIDIOCSPICT, &vpic) < 0 ||
       ioctl(fd, VIDIOCGPICT, &vpic) < 0 ||
       vpic.palette != VIDEO_PALETTE_RGB24) {
        fprintf(stderr,
                "ERROR: unable to reset to previously set RGB24 palette?!\n"
                "    this is an unexpected driver communication error\n");
        return(1);
    }

    /* map camera image to memory */
    struct video_mbuf vbuf;
    if(ioctl(fd, VIDIOCGMBUF, &vbuf) < 0) {
        perror("ERROR VIDIOCGMBUF");
        fprintf(stderr,
                "    device does not support mmap capture interface\n"
                "    this program currently requires mmap capture support\n"
                "      - make sure you have the latest drivers\n");
        return(1);
    }

    printf("memory buffer information:\n"
           "    size      : %08x\n"
           "    frames    : %d\n",
            vbuf.size, vbuf.frames);
    for(i = 0; i < vbuf.frames; i++)
        printf("        [%02d]  : %08x\n", i, vbuf.offsets[i]);

    if(!vbuf.frames) {
        fprintf(stderr,
                "ERROR: no frames are supported by device?!\n"
                "    maybe device does not support mmap capture interface\n"
                "    this program currently requires mmap capture support\n"
                "      - make sure you have the latest drivers\n");
        return(0);
    }

    printf("\nattempting to map memory buffer...\n");
    void *image = mmap(0, vbuf.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(image == MAP_FAILED) {
        perror("ERROR: mmap");
        fprintf(stderr, "    failed to map memory?!\n");
        return(1);
    }
    printf("memory buffer mapped at %p\n\n", image);

    /* now try capturing an image */
    struct video_mmap vmap;
    vmap.frame = 0;
    vmap.height = vwin.height;
    vmap.width = vwin.width;
    vmap.format = vpic.palette;
    printf("initiating capture...\n");
    if(ioctl(fd, VIDIOCMCAPTURE, &vmap) < 0) {
        perror("ERROR VIDIOCMCAPTURE");
        fprintf(stderr, "    failed to initiate image capture?\n");
        return(1);
    }
    printf("waiting for image...\n");
    if(ioctl(fd, VIDIOCSYNC, &vmap.frame) < 0) {
        perror("ERROR VIDIOCSYNC");
        fprintf(stderr, "    failed to capture image\n");
        return(1);
    }

    printf("image capture successful!\n\n"
           "video support verified\n\n");

    return(0);
}
