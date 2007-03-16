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

#include <Magick++.h>
#include <iostream>
#include <zebra.h>

using namespace std;
using namespace Magick;
using namespace zebra;

int display = 0;


class ImageHandler : public Decoder::Handler {
    virtual void decode_callback (Decoder &decoder)
    {
        if(decoder.get_type() > ZEBRA_PARTIAL)
            cout << decoder.get_symbol_name() << decoder.get_addon_name()
                 << ": " << decoder.get_data_string() << endl;
    }
};

ImageHandler handler;
Decoder decoder;
Scanner scanner(decoder);

void scan_image (const char *filename)
{
    Image image;
    image.read(filename);

    unsigned width = image.columns();
    unsigned height = image.rows();

    Color red("red");
    image.modifyImage();

    // extract image pixels
    Pixels view(image);
    PixelPacket *pxp = view.get(0, (height + 1) / 2, width, 1);
    ColorYUV y;
    for(unsigned i = 0; i < width; i++) {
        y = *pxp;
        scanner << (int)(y.y() * 0x100);
        *pxp++ = red;
    }
    view.sync();
    int iy = (int)(y.y() * .75) * 0x100;
    scanner << iy << iy << iy; /* flush scan FIXME? */

    if(display) {
#if (MagickLibVersion >= 0x632)
        image.display();
#else
        // workaround for "no window with specified ID exists" bug
        // ref http://www.imagemagick.org/discourse-server/viewtopic.php?t=6315
        // fixed in 6.3.1-25
        char c = image.imageInfo()->filename[0];
        image.imageInfo()->filename[0] = 0;
        image.display();
        image.imageInfo()->filename[0] = c;
#endif
    }
}

int usage (int rc, const char *msg = NULL)
{
    ostream &out = (rc) ? cerr : cout;
    if(msg)
        out << msg << endl;
    out <<
        "usage: zebraimg [-h|-d|--display|--nodisplay] <image>..." << endl;
    return(rc);
}

int main (int argc, const char *argv[])
{
    decoder.set_handler(handler);

    int num_images = 0;
    for(int i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-h") ||
           !strcmp(argv[i], "--help"))
            return(usage(0));
        if(!strcmp(argv[i], "-d") ||
           !strcmp(argv[i], "--display"))
            display = 1;
        else if(!strcmp(argv[i], "--nodisplay"))
            display = 0;
        else {
            num_images++;
            scan_image(argv[i]);
        }
    }
    if(!num_images)
        return(usage(1, "ERROR: specify image file(s) to scan"));

    return(0);
}
