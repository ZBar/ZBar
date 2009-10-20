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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif
#ifdef _WIN32
# include <io.h>
# include <fcntl.h>
#endif
#include <assert.h>

#include <zbar.h>
#include <wand/MagickWand.h>

/* in 6.4.5.4 MagickGetImagePixels changed to MagickExportImagePixels.
 * (still not sure this check is quite right...
 *  how does MagickGetAuthenticImagePixels fit in?)
 * ref http://bugs.gentoo.org/247292
 */
#if MagickLibVersion < 0x645
# define MagickExportImagePixels MagickGetImagePixels
#endif

static const char *note_usage =
    "usage: zbarimg [options] <image>...\n"
    "\n"
    "scan and decode bar codes from one or more image files\n"
    "\n"
    "options:\n"
    "    -h, --help      display this help text\n"
    "    --version       display version information and exit\n"
    "    -q, --quiet     minimal output, only print decoded symbol data\n"
    "    -v, --verbose   increase debug output level\n"
    "    --verbose=N     set specific debug output level\n"
    "    -d, --display   enable display of following images to the screen\n"
    "    -D, --nodisplay disable display of following images (default)\n"
    "    --xml, --noxml  enable/disable XML output format\n"
    "    --raw           output decoded symbol data without symbology prefix\n"
    "    -S<CONFIG>[=<VALUE>], --set <CONFIG>[=<VALUE>]\n"
    "                    set decoder/scanner <CONFIG> to <VALUE> (or 1)\n"
    // FIXME overlay level
    "\n"
    ;

static const char *warning_not_found =
    "\n"
    "WARNING: barcode data was not detected in some image(s)\n"
    "  things to check:\n"
    "    - is the barcode type supported?"
    "  currently supported symbologies are:\n"
    "      EAN/UPC (EAN-13, EAN-8, UPC-A, UPC-E, ISBN-10, ISBN-13),\n"
    "      Code 128, Code 39 and Interleaved 2 of 5\n"
    "    - is the barcode large enough in the image?\n"
    "    - is the barcode mostly in focus?\n"
    "    - is there sufficient contrast/illumination?\n"
    "\n";

static const char *xml_head =
    "<barcodes xmlns='http://zbar.sourceforge.net/2008/barcode'>\n";
static const char *xml_foot =
    "</barcodes>\n";

static int notfound = 0, exit_code = 0;
static int num_images = 0, num_symbols = 0;
static int xmllvl = 0;

char *xmlbuf = NULL;
unsigned xmlbuflen = 0;

static zbar_processor_t *processor = NULL;

static inline int dump_error(MagickWand *wand)
{
    char *desc;
    ExceptionType severity;
    desc = MagickGetException(wand, &severity);

    if(severity >= FatalErrorException)
        exit_code = 2;
    else if(severity >= ErrorException)
        exit_code = 1;
    else
        exit_code = 0;

    static const char *sevdesc[] = { "WARNING", "ERROR", "FATAL" };
    fprintf(stderr, "%s: %s\n", sevdesc[exit_code], desc);

    MagickRelinquishMemory(desc);
    return(exit_code);
}

static int scan_image (const char *filename)
{
    if(exit_code == 3)
        return(-1);

    int found = 0;
    MagickWand *images = NewMagickWand();
    if(!MagickReadImage(images, filename) && dump_error(images))
        return(-1);

    unsigned seq, n = MagickGetNumberImages(images);
    for(seq = 0; seq < n; seq++) {
        if(exit_code == 3)
            return(-1);

        if(!MagickSetIteratorIndex(images, seq) && dump_error(images))
            return(-1);

        zbar_image_t *zimage = zbar_image_create();
        assert(zimage);
        zbar_image_set_format(zimage, *(unsigned long*)"Y800");

        int width = MagickGetImageWidth(images);
        int height = MagickGetImageHeight(images);
        zbar_image_set_size(zimage, width, height);

        // extract grayscale image pixels
        // FIXME color!! ...preserve most color w/422P
        // (but only if it's a color image)
        size_t bloblen = width * height;
        unsigned char *blob = malloc(bloblen);
        zbar_image_set_data(zimage, blob, bloblen, zbar_image_free_data);

        if(!MagickExportImagePixels(images, 0, 0, width, height,
                                    "I", CharPixel, blob))
            return(-1);

        if(xmllvl == 1) {
            xmllvl++;
            printf("<source href='%s'>\n", filename);
        }

        zbar_process_image(processor, zimage);

        // output result data
        const zbar_symbol_t *sym = zbar_image_first_symbol(zimage);
        for(; sym; sym = zbar_symbol_next(sym)) {
            zbar_symbol_type_t typ = zbar_symbol_get_type(sym);
            if(typ == ZBAR_PARTIAL)
                continue;
            else if(!xmllvl)
                printf("%s%s:%s\n",
                       zbar_get_symbol_name(typ),
                       zbar_get_addon_name(typ),
                       zbar_symbol_get_data(sym));
            else if(xmllvl < 0)
                printf("%s\n", zbar_symbol_get_data(sym));
            else {
                if(xmllvl < 3) {
                    xmllvl++;
                    printf("<index num='%u'>\n", seq);
                }
                zbar_symbol_xml(sym, &xmlbuf, &xmlbuflen);
                printf("%s\n", xmlbuf);
            }
            found++;
            num_symbols++;
        }
        if(xmllvl > 2) {
            xmllvl--;
            printf("</index>\n");
        }
        fflush(stdout);

        zbar_image_destroy(zimage);

        num_images++;
        if(zbar_processor_is_visible(processor)) {
            int rc = zbar_processor_user_wait(processor, -1);
            if(rc < 0 || rc == 'q' || rc == 'Q')
                exit_code = 3;
        }
    }

    if(xmllvl > 1) {
        xmllvl--;
        printf("</source>\n");
    }

    if(!found)
        notfound++;

    DestroyMagickWand(images);
    return(0);
}

int usage (int rc,
           const char *msg,
           const char *arg)
{
    FILE *out = (rc) ? stderr : stdout;
    if(msg) {
        fprintf(out, "%s", msg);
        if(arg)
            fprintf(out, "%s", arg);
        fprintf(out, "\n\n");
    }
    fprintf(out, "%s", note_usage);
    return(rc);
}

static inline int parse_config (const char *cfgstr, const char *arg)
{
    if(!cfgstr || !cfgstr[0])
        return(usage(1, "ERROR: need argument for option: ", arg));

    if(zbar_processor_parse_config(processor, cfgstr))
        return(usage(1, "ERROR: invalid configuration setting: ", cfgstr));

    return(0);
}

int main (int argc, const char *argv[])
{
    // option pre-scan
    int quiet = 0;
    int display = 0;
    int i, j;
    for(i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if(arg[0] != '-')
            // first pass, skip images
            num_images++;
        else if(arg[1] != '-')
            for(j = 1; arg[j]; j++) {
                if(arg[j] == 'S') {
                    if(!arg[++j] && ++i >= argc)
                        /* FIXME parse check */
                        return(parse_config("", "-S"));
                    break;
                }
                switch(arg[j]) {
                case 'h': return(usage(0, NULL, NULL));
                case 'q': quiet = 1; break;
                case 'v': zbar_increase_verbosity(); break;
                case 'd': display = 1; break;
                case 'D': break;
                default:
                    return(usage(1, "ERROR: unknown bundled option: -",
                                 arg + j));
                }
            }
        else if(!strcmp(arg, "--help"))
            return(usage(0, NULL, NULL));
        else if(!strcmp(arg, "--version")) {
            printf("%s\n", PACKAGE_VERSION);
            return(0);
        }
        else if(!strcmp(arg, "--quiet")) {
            quiet = 1;
            argv[i] = NULL;
        }
        else if(!strcmp(arg, "--verbose"))
            zbar_increase_verbosity();
        else if(!strncmp(arg, "--verbose=", 10))
            zbar_set_verbosity(strtol(argv[i] + 10, NULL, 0));
        else if(!strcmp(arg, "--display"))
            display++;
        else if(!strcmp(arg, "--nodisplay") ||
                !strcmp(arg, "--set") ||
                !strcmp(arg, "--xml") ||
                !strcmp(arg, "--noxml") ||
                !strcmp(arg, "--raw") ||
                !strncmp(arg, "--set=", 6))
            continue;
        else if(!strcmp(arg, "--")) {
            num_images += argc - i - 1;
            break;
        }
        else
            return(usage(1, "ERROR: unknown option: ", arg));
    }

    if(!num_images)
        return(usage(1, "ERROR: specify image file(s) to scan", NULL));
    num_images = 0;

    MagickWandGenesis();

    processor = zbar_processor_create(0);
    assert(processor);
    if(zbar_processor_init(processor, NULL, display)) {
        zbar_processor_error_spew(processor, 0);
        return(1);
    }

    for(i = 1; i < argc; i++) {
        const char *arg = argv[i];
        if(!arg)
            continue;

        if(arg[0] != '-') {
            if(scan_image(arg))
                return(exit_code);
        }
        else if(arg[1] != '-')
            for(j = 1; arg[j]; j++) {
                if(arg[j] == 'S') {
                    if((arg[++j])
                       ? parse_config(arg + j, "-S")
                       : parse_config(argv[++i], "-S"))
                        return(1);
                    break;
                }
                switch(arg[j]) {
                case 'd': zbar_processor_set_visible(processor, 1);  break;
                case 'D': zbar_processor_set_visible(processor, 0);  break;
                }
            }
        else if(!strcmp(arg, "--display"))
            zbar_processor_set_visible(processor, 1);
        else if(!strcmp(arg, "--nodisplay"))
            zbar_processor_set_visible(processor, 0);
        else if(!strcmp(arg, "--xml")) {
            if(xmllvl < 1) {
                xmllvl = 1;
#ifdef _WIN32
                fflush(stdout);
                _setmode(_fileno(stdout), _O_BINARY);
#endif
                printf("%s", xml_head);
            }
        }
        else if(!strcmp(arg, "--noxml") || !strcmp(arg, "--raw")) {
            if(xmllvl > 0) {
                xmllvl = 0;
                printf("%s", xml_foot);
                fflush(stdout);
#ifdef _WIN32
                _setmode(_fileno(stdout), _O_TEXT);
#endif
            }
            if(!strcmp(arg, "--raw")) {
                xmllvl = -1;
#ifdef _WIN32
                fflush(stdout);
                _setmode(_fileno(stdout), _O_BINARY);
#endif
            }
        }
        else if(!strcmp(arg, "--set")) {
            if(parse_config(argv[++i], "--set"))
                return(1);
        }
        else if(!strncmp(arg, "--set=", 6)) {
            if(parse_config(arg + 6, "--set="))
                return(1);
        }
        else if(!strcmp(arg, "--"))
            break;
    }
    for(i++; i < argc; i++)
        if(scan_image(argv[i]))
            return(exit_code);

    /* ignore quit during last image */
    if(exit_code == 3)
        exit_code = 0;

    if(xmllvl > 0) {
        xmllvl = -1;
        printf("%s", xml_foot);
        fflush(stdout);
    }

    if(xmlbuf)
        free(xmlbuf);

    if(num_images && !quiet && xmllvl <= 0) {
        fprintf(stderr, "scanned %d barcode symbols from %d images",
                num_symbols, num_images);
#ifdef HAVE_SYS_TIMES_H
#ifdef HAVE_UNISTD_H
        long clk_tck = sysconf(_SC_CLK_TCK);
        struct tms tms;
        if(clk_tck > 0 && times(&tms) >= 0) {
            double secs = tms.tms_utime + tms.tms_stime;
            secs /= clk_tck;
            fprintf(stderr, " in %.2g seconds\n", secs);
        }
#endif
#endif
        fprintf(stderr, "\n");
        if(notfound)
            fprintf(stderr, "%s", warning_not_found);
    }
    if(num_images && notfound && !exit_code)
        exit_code = 4;

    zbar_processor_destroy(processor);
    MagickWandTerminus();
    return(exit_code);
}
