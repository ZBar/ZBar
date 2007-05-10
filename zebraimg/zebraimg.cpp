//------------------------------------------------------------------------
//  Copyright 2007 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the Zebra Barcode Library.
//
//  The Zebra Barcode Library is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser Public License as
//  published by the Free Software Foundation; either version 2.1 of
//  the License, or (at your option) any later version.
//
//  The Zebra Barcode Library is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
//  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser Public License for more details.
//
//  You should have received a copy of the GNU Lesser Public License
//  along with the Zebra Barcode Library; if not, write to the Free
//  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
//  http://sourceforge.net/projects/zebra
//------------------------------------------------------------------------

#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <Magick++.h>
#include <iostream>
#include <assert.h>

#include <zebra.h>

/* wand/wand-config.h defines these conflicting values :| */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#include "config.h"

using namespace std;
using namespace Magick;
using namespace zebra;

int display = 0, num_images = 0;

Decoder decoder;
Scanner scanner(decoder);
ImageWalker walker;
const PixelPacket *rd_pxp;
PixelPacket *wr_pxp;

Color red("red");

class PixelHandler : public ImageWalker::Handler {
    virtual char walker_callback (ImageWalker &, void *pixel)
    {
        ColorYUV y;
        y = *(rd_pxp + (uintptr_t)pixel);
        scanner << (int)(y.y() * 0x100);
        *(wr_pxp + (uintptr_t)pixel) = red;
        return(0);
    }
} pixel_handler;

class SymbolHandler : public Decoder::Handler {
    virtual void decode_callback (Decoder &decoder)
    {
        if(decoder.get_type() > ZEBRA_PARTIAL)
            cout << decoder.get_symbol_name() << decoder.get_addon_name()
                 << ": " << decoder.get_data_string() << endl;
    }
} symbol_handler;

void scan_image (const char *filename)
{
    Image image;
    image.read(filename);
    Image wr_img = image;

    unsigned width = image.columns();
    unsigned height = image.rows();
    walker.set_size(width, height);

    wr_img.modifyImage();

    // extract image pixels
    Pixels rd_view(image);
    rd_pxp = rd_view.getConst(0, 0, width, height);
    Pixels wr_view(wr_img);
    wr_pxp = wr_view.get(0, 0, width, height);
    walker.walk(0);
    assert(*rd_pxp != red);

    wr_view.sync();
    scanner << 0 << 0 << 0; /* flush scan FIXME? */

    if(display) {
#if (MagickLibVersion >= 0x632)
        wr_img.display();
#else
        // workaround for "no window with specified ID exists" bug
        // ref http://www.imagemagick.org/discourse-server/viewtopic.php?t=6315
        // fixed in 6.3.1-25
        char c = wr_img.imageInfo()->filename[0];
        wr_img.imageInfo()->filename[0] = 0;
        wr_img.display();
        wr_img.imageInfo()->filename[0] = c;
#endif
    }
    num_images++;
}

int usage (int rc, const char *msg = NULL)
{
    ostream &out = (rc) ? cerr : cout;
    if(msg)
        out << msg << endl;
    out << "usage: zebraimg [-h|--help|-d|--display|--nodisplay] <image>..." << endl
        << endl
        << "scan and decode bar codes from one or more image files" << endl
        << endl
        << "options:" << endl
        << "    -h, --help     display this help text" << endl
        << "    --version      display version information and exit" << endl
        << "    -d, --display  enable display of following images to the screen" << endl
        << "    --nodisplay    disable display of following images (default)" << endl
        << endl;
    return(rc);
}

int main (int argc, const char *argv[])
{
    decoder.set_handler(symbol_handler);
    walker.set_handler(pixel_handler);

    int i;
    for(i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-h") ||
           !strcmp(argv[i], "--help"))
            return(usage(0));
        if(!strcmp(argv[i], "--version")) {
            cout << PACKAGE_VERSION << endl;
            return(0);
        }
        if(!strcmp(argv[i], "-d") ||
           !strcmp(argv[i], "--display"))
            display = 1;
        else if(!strcmp(argv[i], "--nodisplay"))
            display = 0;
        else if(!strcmp(argv[i], "--"))
            break;
        else
            scan_image(argv[i]);
    }
    for(i++; i < argc; i++)
        scan_image(argv[i]);

    if(!num_images)
        return(usage(1, "ERROR: specify image file(s) to scan"));

    return(0);
}
