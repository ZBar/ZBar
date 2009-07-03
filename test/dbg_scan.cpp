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

#include <Magick++.h>
#include <iostream>
#include <fstream>
#include <libgen.h>
#include <zbar.h>

using namespace std;
using namespace zbar;

#ifndef ZBAR_FIXED
# define ZBAR_FIXED 5
#endif

#define ZBAR_FRAC (1 << ZBAR_FIXED)

Decoder decoder;
Scanner scanner;

/* undocumented API for drawing cutesy debug graphics */
extern "C" void zbar_scanner_get_state(const zbar_scanner_t *scn,
                                        unsigned *x,
                                        unsigned *cur_edge,
                                        unsigned *last_edge,
                                        int *y0,
                                        int *y1,
                                        int *y2,
                                        int *y1_thresh);

void scan_image (const char *filename)
{
    scanner.reset();
    // normally scanner would reset associated decoder,
    // but this debug program connects them manually
    // (to make intermediate state more readily available)
    // so decoder must also be reset manually
    decoder.reset();

    Magick::Image image;
    image.read(filename);
    string file = image.baseFilename();
    size_t baseidx = file.rfind('/');
    if(baseidx != string::npos)
        file = file.substr(baseidx + 1, file.length() - baseidx - 1);
    ofstream svg((file + ".svg").c_str());

    unsigned inwidth = image.columns();
    unsigned width = inwidth + 3;
    unsigned height = image.rows();
    unsigned midy = height / 2;
    cerr << "x+: " << midy << endl;

    image.crop(Magick::Geometry(inwidth, 1, 0, midy));

    svg << "<?xml version='1.0'?>" << endl
        << "<!DOCTYPE svg PUBLIC '-//W3C//DTD SVG 1.1//EN'"
        << " 'http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd'>" << endl
        << "<svg version='1.1' id='top'"
        << " width='10in' height='6in' preserveAspectRatio='xMinYMid slice'"
        << " overflow='visible' viewBox='0,0 " << width * 2 << ",384'"
        << " xmlns:xlink='http://www.w3.org/1999/xlink'"
        << " xmlns='http://www.w3.org/2000/svg'>" << endl
        << "<defs><style type='text/css'><![CDATA[" << endl
        << "  * { stroke-linejoin: round; stroke-linecap: round;"
        <<      " stroke-width: .1; text-anchor: middle;"
        <<      " image-rendering: optimizeSpeed;"
        <<      " font-size: 6; font-weight: bold }" << endl
        << "  path { fill: none }" << endl
        << "  #zero { stroke: #00f }" << endl
        << "  #edges { stroke: #f00 }" << endl
        << "  #cur-edge { stroke: #f44 }" << endl
        << "  #raw { stroke: orange }" << endl
        << "  #y0 { stroke: yellow }" << endl
        << "  #y1 { stroke: #0c0 }" << endl
        << "  #y2 { stroke: #0aa }" << endl
        << "  .y1thr { stroke: #f0f }" << endl
        << "  rect.bar { fill: black }" << endl
        << "  text.bar { fill: white }" << endl
        << "  rect.space { fill: white }" << endl
        << "  text.space { fill: black }" << endl
        << "  text.data { fill: #44f; font-size: 16 }" << endl
        << "]]></style></defs>" << endl
        << "<image width='" << inwidth * 2 << "' height='384'"
        << " preserveAspectRatio='none'"
        << " xlink:href='" << file << ".png'/>" << endl
        << "<g transform='translate(1,384) scale(2,-.5)'>" << endl;

    // brute force
    unsigned raw[inwidth];
    {
        // extract scan from image pixels
        image.modifyImage();
        Magick::Pixels view(image);
        Magick::PixelPacket *pxp = view.get(0, 0, inwidth, 1);
        Magick::ColorYUV y;
        double max = 0;
        svg << "<path id='raw' d='M";
        unsigned i;
        for(i = 0; i < inwidth; i++, pxp++) {
            y = *pxp;
            if(max < y.y())
                max = y.y();
            raw[i] = (unsigned)(y.y() * 0x100);
            svg << ((i != 1) ? " " : " L ") << i << "," << raw[i];
            y.u(0);
            y.v(0);
            *pxp = y;
        }
        view.sync();
        svg << "'/>" << endl
            << "</g>" << endl;
    }
    image.depth(8);
    image.write(file + ".png");

    // process scan and capture calculated values
    unsigned cur_edge[width], last_edge[width];
    int y0[width], y1[width], y2[width], y1_thr[width];

    svg << "<g transform='translate(-3)'>" << endl;
    for(unsigned i = 0; i < width; i++) {
        int edge;
        if(i < inwidth)
            edge = scanner.scan_y(raw[i]);
        else
            edge = scanner.flush();

        unsigned x;
        zbar_scanner_get_state(scanner, &x,
                               &cur_edge[i], &last_edge[i],
                               &y0[i], &y1[i], &y2[i], &y1_thr[i]);
        if(edge) {
            unsigned w = scanner.get_width();
            if(w)
                svg << "<rect x='" << (2. * (last_edge[i] - w) / ZBAR_FRAC)
                    << "' width='" << (w * 2. / ZBAR_FRAC)
                    << "' height='32' class='"
                    << (scanner.get_color() ? "space" : "bar") << "'/>" << endl
                    << "<text transform='translate("
                    << ((2. * last_edge[i] - w) / ZBAR_FRAC) - 3
                    << ",16) rotate(90)' class='"
                    << (scanner.get_color() ? "space" : "bar") << "'>" << endl
                    << w << "</text>" << endl;
            zbar_symbol_type_t sym = decoder.decode_width(w);
            if(sym > ZBAR_PARTIAL) {
                svg << "<text transform='translate("
                    << (2. * (last_edge[i] + w) / ZBAR_FRAC)
                    << ",208) rotate(90)' class='data'>"
                    << decoder.get_data_string() << "</text>" << endl;
            }
        }
        else if((!i)
                ? last_edge[i]
                : last_edge[i] == last_edge[i - 1])
            last_edge[i] = 0;
    }

    svg << "</g>" << endl
        << "<g transform='translate(-3,384) scale(2,-.5)'>" << endl
        << "<path id='edges' d='";
    for(unsigned i = 0; i < width; i++)
        if(last_edge[i])
            svg << " M" << ((double)last_edge[i] / ZBAR_FRAC) << ",0v768";
    svg << "'/>" << endl
        << "</g>" << endl
        << "<g transform='translate(-1,384) scale(2,-.5)'>" << endl
        << "<path id='y0' d='M";
    for(unsigned i = 0; i < width; i++)
        svg << ((i != 1) ? " " : " L ") << i << "," << y0[i];
    svg << "'/>" << endl
        << "</g>" << endl;

    svg << "<g transform='translate(-1,128) scale(2,-1)'>" << endl
        << "<line id='zero' x2='" << width << "'/>" << endl
        << "<path id='cur-edge' d='";
    for(unsigned i = 1; i < width - 1; i++)
        if(!last_edge[i + 1] && (cur_edge[i] != cur_edge[i + 1]))
            svg << " M" << ((double)cur_edge[i] / ZBAR_FRAC) - 1 << ",-32v64";
    svg << "'/>" << endl
        << "<path class='y1thr' d='M";
    for(unsigned i = 0; i < width; i++)
        svg << ((i != 1) ? " " : " L ") << i << "," << y1_thr[i];
    svg << "'/>" << endl
        << "<path class='y1thr' d='M";
    for(unsigned i = 0; i < width; i++)
        svg << ((i != 1) ? " " : " L ") << i << "," << -y1_thr[i];
    svg << "'/>" << endl
        << "<path id='y1' d='M";
    for(unsigned i = 0; i < width; i++)
        svg << ((i != 1) ? " " : " L ") << (i - 0.5) << "," << y1[i];
    svg << "'/>" << endl
        << "<path id='y2' d='M";
    for(unsigned i = 0; i < width; i++)
        svg << ((i != 1) ? " " : " L ") << i << "," << y2[i];
    svg << "'/>" << endl
        << "</g>" << endl;

    svg << "</svg>" << endl;
}

int main (int argc, const char *argv[])
{
    if(argc < 2) {
        cerr << "ERROR: specify image file(s) to scan" << endl;
        return(1);
    }

    for(int i = 1; i < argc; i++)
        scan_image(argv[i]);
    return(0);
}
