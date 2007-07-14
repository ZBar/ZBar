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

/* wand/wand-config.h defines these conflicting values :| */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include "config.h"
#include <iostream>
#include <assert.h>
#include <string>
#include <list>

#include <zebra.h>

using namespace std;
using namespace Magick;
using namespace zebra;

int display = 0, num_images = 0;

ImageScanner scanner;

static inline void overlay_grid (Image& img)
{
    unsigned width = img.columns();
    unsigned height = img.rows();
    list<Drawable> overlay;
    overlay.push_back(DrawableStrokeWidth(1));
    overlay.push_back(DrawableStrokeColor(ColorGray(1)));
    overlay.push_back(DrawableFillOpacity(0));
    overlay.push_back(DrawableStrokeOpacity(.15));
    for(unsigned i = 8; i < width; i += 16)
        overlay.push_back(DrawableLine(i, 0, i, height - 1));
    for(unsigned i = 8; i < height; i += 16)
        overlay.push_back(DrawableLine(0, i, width, i));
    img.draw(overlay);
}

static inline void overlay_symbol (Image& img,
                                   Symbol& sym)
{
    list<Drawable> overlay;
    overlay.push_back(DrawableStrokeColor(ColorRGB(1,0,0)));
    overlay.push_back(DrawableFillOpacity(0));
    overlay.push_back(DrawableStrokeOpacity(.5));
    overlay.push_back(DrawableStrokeWidth(3));

    // FIXME eventually will always have at least 4 points
    int n_pts = sym.get_location_size();
    assert(n_pts);
    if(n_pts == 1) {
        int x = sym.get_location_x(0);
        int y = sym.get_location_y(0);
        cerr << "(" << x << "," << y << ")" << endl;
        overlay.push_back(DrawablePoint(x, y));
        overlay.push_back(DrawableCircle(x, y, x - 5, y - 5));
    }
    else if(n_pts == 2)
        overlay.push_back(DrawableLine(sym.get_location_x(0),
                                       sym.get_location_y(0),
                                       sym.get_location_x(1),
                                       sym.get_location_y(1)));
    else {
        list<Coordinate> poly;
        for(int j = 0; j < n_pts; j++)
            poly.push_back(Coordinate(sym.get_location_x(j),
                                      sym.get_location_y(j)));
        overlay.push_back(DrawablePolygon(poly));
    }
    img.draw(overlay);
}

static int scan_image (const char *filename)
{
    Image disp_img;
    try {
        disp_img.read(filename);
    }
    catch (Exception &e) {
        cout << "ERROR: " << e.what() << endl
             << "while reading " << filename << endl;
        return(1);
    }

    {
        // extract grayscale image pixels
        Image scan_img = disp_img;
        scan_img.modifyImage();
        Blob scan_data;
        scan_img.write(&scan_data, "GRAY", 8);
        unsigned width = disp_img.columns();
        unsigned height = disp_img.rows();
        assert(scan_data.length() == width * height);
        scanner.set_size(width, height);
        scanner.scan_y(scan_data.data());
    }

    if(display) {
        // FIXME overlay optional
        disp_img.modifyImage();
        disp_img.type(TrueColorType);
        overlay_grid(disp_img);
    }

    // output result data
    for(int i = 0; i < scanner.get_result_size(); i++) {
        Symbol symbol = scanner.get_result(i);
        cout << (string(symbol.get_type_name()) + 
                 string(symbol.get_addon_name()) +
                 ":" + symbol.get_data()) << endl;
        cout.flush();

        if(display)
            overlay_symbol(disp_img, symbol);
    }

    if(display) {
#if (MagickLibVersion >= 0x632)
        disp_img.display();
#else
        // workaround for "no window with specified ID exists" bug
        // ref http://www.imagemagick.org/discourse-server/viewtopic.php?t=6315
        // fixed in 6.3.1-25
        char c = disp_img.imageInfo()->filename[0];
        disp_img.imageInfo()->filename[0] = 0;
        disp_img.display();
        disp_img.imageInfo()->filename[0] = c;
#endif
    }
    num_images++;
    return(0);
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
    int i, rc = 0;
    for(i = 1; i < argc; i++) {
        if(!strcmp(argv[i], "-h") ||
           !strcmp(argv[i], "--help"))
            return(usage(rc));
        if(!strcmp(argv[i], "--version")) {
            cout << PACKAGE_VERSION << endl;
            return(rc);
        }
        if(!strcmp(argv[i], "-d") ||
           !strcmp(argv[i], "--display"))
            display = 1;
        else if(!strcmp(argv[i], "--nodisplay"))
            display = 0;
        else if(!strcmp(argv[i], "--"))
            break;
        else
            rc |= scan_image(argv[i]);
    }
    for(i++; i < argc; i++)
        rc |= scan_image(argv[i]);

    if(!num_images)
        return(usage(1, "ERROR: specify image file(s) to scan"));

    return(rc);
}
