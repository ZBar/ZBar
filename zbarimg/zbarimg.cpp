//------------------------------------------------------------------------
//  Copyright 2007-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the ZBar Bar Code Reader.
//
//  The ZBar Bar Code Reader is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser Public License as
//  published by the Free Software Foundation; either version 2.1 of
//  the License, or (at your option) any later version.
//
//  The ZBar Bar Code Reader is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
//  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser Public License for more details.
//
//  You should have received a copy of the GNU Lesser Public License
//  along with the ZBar Bar Code Reader; if not, write to the Free
//  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
//  http://sourceforge.net/projects/zbar
//------------------------------------------------------------------------

#include <Magick++.h>

/* wand/wand-config.h (or magick/deprecate.h?)
 * defines these conflicting values :|
 */
#undef PACKAGE
#undef VERSION
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include <config.h>
#include <iostream>
#include <sstream>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_SYS_TIMES_H
# include <sys/times.h>
#endif
#include <assert.h>
#include <string>
#include <list>

#include <zbar.h>

using namespace std;
using namespace zbar;

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
    "    -S<CONFIG>[=<VALUE>], --set <CONFIG>[=<VALUE>]\n"
    "                    set decoder/scanner <CONFIG> to <VALUE> (or 1)\n"
    // FIXME overlay level
    ;

static const char *warning_not_found =
    "WARNING: barcode data was not detected in some image(s)\n"
    "  things to check:\n"
    "    - is the barcode type supported?"
    "  currently supported symbologies are:\n"
    "      EAN/UPC (EAN-13, EAN-8, UPC-A, UPC-E, ISBN-10, ISBN-13),\n"
    "      Code 128, Code 39 and Interleaved 2 of 5\n"
    "    - is the barcode large enough in the image?\n"
    "    - is the barcode mostly in focus?\n"
    "    - is there sufficient contrast/illumination?\n";

static const char *xml_head =
    "<barcodes xmlns='http://zbar.sourceforge.net/2008/barcode'>";
static const char *xml_foot =
    "</barcodes>";

static int notfound = 0;
static int num_images = 0, num_symbols = 0;
static int xmllvl = 0;

static Processor *processor = NULL;

static void scan_image (const std::string& filename)
{
    bool found = false;
    list<Magick::Image> images;
    Magick::readImages(&images, filename);
    int seq = 0;
    for(list<Magick::Image>::iterator image = images.begin();
        image != images.end();
        ++image, seq++)
    {
        image->modifyImage();

        // extract grayscale image pixels
        // FIXME color!! ...preserve most color w/422P
        // (but only if it's a color image)
        Magick::Blob scan_data;
        image->write(&scan_data, "GRAY", 8);
        unsigned width = image->columns();
        unsigned height = image->rows();
        assert(scan_data.length() == width * height);

        Image zimage(width, height, "Y800",
                     scan_data.data(), scan_data.length());
        processor->process_image(zimage);

        // output result data
        for(Image::SymbolIterator sym = zimage.symbol_begin();
            sym != zimage.symbol_end();
            ++sym)
        {
            if(!xmllvl)
                cout << *sym << endl;
            else {
                if(xmllvl < 2) {
                    xmllvl++;
                    cout << "<source href='" << filename << "'>" << endl;
                }
                if(xmllvl < 3) {
                    xmllvl++;
                    cout << "<index num='" << seq << "'>" << endl;
                }
                cout << sym->xml() << endl;
            }
            found = true;
            num_symbols++;
        }
        if(xmllvl > 2) {
            xmllvl--;
            cout << "</index>" << endl;
        }
        cout.flush();

        num_images++;
        if(processor->is_visible())
            processor->user_wait();
    }
    if(xmllvl > 1) {
        xmllvl--;
        cout << "</source>" << endl;
    }

    if(!found)
        notfound++;
}

int usage (int rc, const string& msg = "")
{
    ostream &out = (rc) ? cerr : cout;
    if(msg.length())
        out << msg << endl << endl;
    out << note_usage << endl;
    return(rc);
}

static inline int parse_config (const string& cfgstr, const string& arg)
{
    if(!cfgstr.length())
        return(usage(1, string("ERROR: need argument for option: " + arg)));

    if(processor->set_config(cfgstr.c_str()))
        return(usage(1, string("ERROR: invalid configuration setting: " +
                               cfgstr)));

    return(0);
}

int main (int argc, const char *argv[])
{
    // option pre-scan
    bool quiet = false;
    bool display = false;
    int i;
    for(i = 1; i < argc; i++) {
        string arg(argv[i]);
        if(arg[0] != '-')
            // first pass, skip images
            num_images++;
        else if(arg[1] != '-') {
            for(int j = 1; arg[j]; j++) {
                if(arg[j] == 'S') {
                    if(!arg[++j] && ++i >= argc)
                        return(parse_config("", "-S"));
                    break;
                }
                switch(arg[j]) {
                case 'h': return(usage(0));
                case 'q': quiet = true; break;
                case 'v': zbar_increase_verbosity(); break;
                case 'd': display = true; break;
                case 'D': break;
                default:
                    return(usage(1, string("ERROR: unknown bundled option: -") +
                                 arg[j]));
                }
            }
        }
        else if(arg == "--help")
            return(usage(0));
        else if(arg == "--version") {
            cout << PACKAGE_VERSION << endl;
            return(0);
        }
        else if(arg == "--quiet") {
            quiet = true;
            argv[i] = NULL;
        }
        else if(arg == "--verbose")
            zbar_increase_verbosity();
        else if(arg.substr(0, 10) == "--verbose=") {
            istringstream scan(arg.substr(10));
            int level;
            scan >> level;
            zbar_set_verbosity(level);
        }
        else if(arg == "--display")
            display = true;
        else if(arg == "--nodisplay" ||
                arg == "--set" ||
                arg == "--xml" ||
                arg == "--noxml" ||
                arg.substr(0, 6) == "--set=")
            continue;
        else if(arg == "--") {
            num_images += argc - i - 1;
            break;
        }
        else
            return(usage(1, "ERROR: unknown option: " + arg));
    }

    if(!num_images)
        return(usage(1, "ERROR: specify image file(s) to scan"));
    num_images = 0;

    try {
        /* process and optionally display images (no video) unthreaded */
        processor = new Processor(false, NULL, display);

        for(i = 1; i < argc; i++) {
            if(!argv[i])
                continue;
            string arg(argv[i]);
            if(arg[0] != '-')
                scan_image(arg);
            else if(arg[1] != '-')
                for(int j = 1; arg[j]; j++) {
                    if(arg[j] == 'S') {
                        if((arg[++j])
                           ? parse_config(arg.substr(j), "-S")
                           : parse_config(string(argv[++i]), "-S"))
                            return(1);
                        break;
                    }
                    switch(arg[j]) {
                    case 'd': processor->set_visible(true);  break;
                    case 'D': processor->set_visible(false);  break;
                    }
                }
            else if(arg == "--display")
                processor->set_visible(true);
            else if(arg == "--nodisplay")
                processor->set_visible(false);
            else if(arg == "--xml") {
                if(xmllvl < 1) {
                    xmllvl++;
                    cout << xml_head << endl;
                }
            }
            else if(arg == "--noxml") {
                if(xmllvl > 0) {
                    xmllvl--;
                    cout << xml_foot << endl;
                }
            }
            else if(arg == "--set") {
                if(parse_config(string(argv[++i]), "--set"))
                    return(1);
            }
            else if(arg.substr(0, 6) == "--set=") {
                if(parse_config(arg.substr(6), "--set="))
                    return(1);
            }
            else if(arg == "--")
                break;
        }
        for(i++; i < argc; i++)
            scan_image(argv[i]);
    }
    catch(Exception &e) {
        cerr << e.what() << endl;
        return(1);
    }
    catch(std::exception &e) {
        cerr << "ERROR: " << e.what() << endl;
        return(1);
    }

    if(xmllvl > 0) {
        xmllvl--;
        cout << xml_foot << endl;
        cout.flush();
    }

    if(num_images && !quiet) {
        cerr << "scanned " << num_symbols << " barcode symbols from "
             << num_images << " images";
#ifdef HAVE_SYS_TIMES_H
#ifdef HAVE_UNISTD_H
        long clk_tck = sysconf(_SC_CLK_TCK);
        struct tms tms;
        if(clk_tck > 0 && times(&tms) >= 0) {
            double secs = tms.tms_utime + tms.tms_stime;
            secs /= clk_tck;
            cerr << " in " << secs << " seconds";
        }
#endif
#endif
        cerr << endl;
        if(notfound)
            cerr << endl << warning_not_found << endl;
    }
    return(0);
}
