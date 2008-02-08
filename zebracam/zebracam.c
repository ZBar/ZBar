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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <zebra.h>

#define BELL "\a"

static const char *note_usage =
    "usage: zebracam [options] [/dev/video?]\n"
    "\n"
    "scan and decode bar codes from a video stream\n"
    "\n"
    "options:\n"
    "    -h, --help      display this help text\n"
    "    --version       display version information and exit\n"
    "    -q, --quiet     disable beep when symbol is decoded\n"
    "    -v, --verbose   increase debug output level\n"
    "    --verbose=N     set specific debug output level\n"
    "    --nodisplay     disable video display window\n"
    // FIXME overlay level
    // FIXME xml output
    "\n";

zebra_processor_t *proc;
int quiet = 0;

static int usage (int rc)
{
    FILE *out = (rc) ? stderr : stdout;
    fprintf(out, note_usage);
    return(rc);
}

void data_handler (zebra_image_t *img, const void *userdata)
{
    const zebra_symbol_t *sym = zebra_image_first_symbol(img);
    assert(sym);
    for(; sym; sym = zebra_symbol_next(sym)) {
        if(zebra_symbol_get_count(sym))
            continue;
        zebra_symbol_type_t type = zebra_symbol_get_type(sym);
        printf("%s%s: %s\n",
               zebra_get_symbol_name(type), zebra_get_addon_name(type),
               zebra_symbol_get_data(sym));
    }
    if(!quiet)
        printf(BELL);
    fflush(stdout);
}

int main (int argc, const char *argv[])
{
    const char *video_device = "/dev/video0";
    int display = 1;
    int i;
    for(i = 1; i < argc; i++) {
        if(argv[i][0] != '-')
            video_device = argv[i];
        else if(argv[i][1] != '-') {
            int j;
            for(j = 1; argv[i][j]; j++) {
                switch(argv[i][j]) {
                case 'h': return(usage(0));
                case 'v': zebra_increase_verbosity(); break;
                case 'q': quiet = 1; break;
                default:
                    fprintf(stderr, "ERROR: unknown bundled option: -%c\n\n",
                            argv[i][j]);
                    return(usage(1));
                }
            }
        }
        else if(!argv[i][2]) {
            if(i < argc - 1)
                video_device = argv[argc - 1];
            break;
        }
        else if(!strcmp(argv[i], "--help"))
            return(usage(0));
        else if(!strcmp(argv[i], "--version"))
            return(printf(PACKAGE_VERSION "\n") <= 0);
        else if(!strcmp(argv[i], "--quiet"))
            quiet = 1;
        else if(!strcmp(argv[i], "--nodisplay"))
            display = 0;
        else if(!strcmp(argv[i], "--verbose"))
            zebra_increase_verbosity();
        else if(!strncmp(argv[i], "--verbose=", 10))
            zebra_set_verbosity(strtol(argv[i] + 10, NULL, 0));
        else {
            fprintf(stderr, "ERROR: unknown option argument: %s\n\n",
                    argv[i]);
            return(usage(1));
        }
    }

    /* setup zebra library standalone processor,
     * threads will be used if available
     */
    proc = zebra_processor_create(1);
    if(!proc) {
        fprintf(stderr, "ERROR: unable to allocate memory?\n");
        return(1);
    }
    zebra_processor_set_data_handler(proc, data_handler, NULL);

    /* open video device, open window */
    if(zebra_processor_init(proc, video_device, display) ||
       /* show window */
       (display && zebra_processor_set_visible(proc, 1)) ||
       /* start video */
       zebra_processor_set_active(proc, 1))
        return(zebra_processor_error_spew(proc, 0));

    /* let the callback handle data */
    int rc;
    while((rc = zebra_processor_user_wait(proc, -1)) >= 0) {
        if(rc == 'q')
            break;
    }

    /* report any errors that aren't "window closed" */
    if(rc && rc != 'q' &&
       zebra_processor_get_error_code(proc) != ZEBRA_ERR_CLOSED)
        return(zebra_processor_error_spew(proc, 0));

    /* free resources (leak check) */
    zebra_processor_destroy(proc);
    return(0);
}
