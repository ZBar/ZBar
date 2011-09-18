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

// NB do not put anything before this header
// it's here to check that we didn't omit any dependencies
#include <zbar.h>

#include <iostream>
#include <sstream>
#include <iomanip>
#include "test_images.h"

bool debug = false;
bool verbose = false;
int errors = 0;
zbar::zbar_symbol_type_t expect_type = zbar::ZBAR_NONE;
std::string expect_data;

template <class T>
inline std::string to_string (const T& t)
{
    std::stringstream ss;
    ss << t;
    return ss.str();
}

static inline int
error (const std::string &msg)
{
    errors++;
    std::cerr << "ERROR: " << msg << std::endl;
    if(debug)
        abort();
    return(-1);
}

static inline int
check_loc (const zbar::Image &img,
           const zbar::Symbol &sym)
{
    int n = 0;
    int w = img.get_width();
    int h = img.get_height();
    for(zbar::Symbol::PointIterator p(sym.point_begin());
        p != sym.point_end();
        ++p, n++)
    {
        zbar::Symbol::Point q(*p);
        if(q.x < 0 || q.x >= w ||
           q.y < 0 || q.y >= h)
            error("location point out of range");
    }
    return(!n);
}

static inline int
check_symbol (const zbar::Image &img,
              const zbar::Symbol &sym)
{
    zbar::zbar_symbol_type_t type(sym.get_type());
    std::string data(sym.get_data());

    bool pass =
        expect_type &&
        type == expect_type &&
        data == expect_data &&
        sym.get_data_length() == expect_data.length() &&
        sym.get_quality() > 4;
    if(pass)
        pass = !check_loc(img, sym);

    if(verbose || !pass)
        std::cerr << "decode Symbol: " << sym << std::endl;

    if(!expect_type)
        error("unexpected");
    else if(!pass)
        error(std::string("expected: ") +
              zbar::zbar_get_symbol_name(expect_type) +
              " " + expect_data);

    expect_type = zbar::ZBAR_NONE;
    expect_data = "";
    return(!pass);
}

static inline int
check_image (const zbar::Image &img)
{
    zbar::SymbolSet syms(img.get_symbols());
    int setn = syms.get_size(), countn = 0;

    int rc = 0;
    for(zbar::SymbolIterator sym(syms.symbol_begin());
        sym != syms.symbol_end();
        ++sym, ++countn)
        rc |= check_symbol(img, *sym);

    if(countn != setn)
        rc |= error("SymbolSet size mismatch: exp=" + to_string(setn) +
                    " act=" + to_string(countn));
    return(rc);
}

static inline void
expect (zbar::zbar_symbol_type_t type,
        std::string data)
{
    if(expect_type)
        error(std::string("missing: ") + zbar_get_symbol_name(expect_type) +
              " " + expect_data);
    expect_type = type;
    expect_data = data;
}

class Handler : public zbar::Image::Handler {
    void image_callback(zbar::Image &img);
};

void
Handler::image_callback (zbar::Image &img)
{
    bool unexpected = !expect_type;
    if(unexpected)
        error("unexpected image callback");
    check_image(img);
}

static inline int
test_processor ()
{
    // create processor w/no video and no window
    zbar::Processor proc(debug, NULL);
    Handler handler;
    proc.set_handler(handler);
    if(debug) {
        proc.set_visible();
        proc.user_wait();
    }

    // generate barcode test image
    zbar::Image rgb3(0, 0, "RGB3");

    // test cast to C image
    if(test_image_ean13(rgb3))
        error("failed to generate image");

    // test decode
    expect(zbar::ZBAR_EAN13, test_image_ean13_data);
    proc.process_image(rgb3);
    if(debug)
        proc.user_wait();

    expect(zbar::ZBAR_EAN13, test_image_ean13_data);
    check_image(rgb3);

    if(rgb3.get_format() != zbar_fourcc('R','G','B','3'))
        error("image format mismatch");

    expect(zbar::ZBAR_NONE, "");
    proc.set_config(zbar::ZBAR_EAN13,
                    zbar::ZBAR_CFG_ENABLE,
                    false);
    proc.process_image(rgb3);
    check_image(rgb3);
    if(debug)
        proc.user_wait();

    proc.set_config("ean13.en");
    expect(zbar::ZBAR_EAN13, test_image_ean13_data);
    proc << rgb3;
    expect(zbar::ZBAR_EAN13, test_image_ean13_data);
    check_image(rgb3);
    if(debug)
        proc.user_wait();

    {
        zbar::Image grey(rgb3.convert(zbar_fourcc('G','R','E','Y')));
        expect(zbar::ZBAR_EAN13, test_image_ean13_data);
        proc << grey;

        zbar::Image y800 = grey.convert("Y800");
        expect(zbar::ZBAR_EAN13, test_image_ean13_data);
        proc << y800;
    }
    if(debug)
        // check image data retention
        proc.user_wait();

    expect(zbar::ZBAR_NONE, "");
    return(0);
}

int main (int argc, char **argv)
{
    debug = (argc > 1 && std::string(argv[1]) == "-d");
    verbose = (debug || (argc > 1 && std::string(argv[1]) == "-v"));

    if(test_processor()) {
        error("ERROR: Processor test FAILED");
        return(2);
    }

    if(test_image_check_cleanup())
        error("cleanup failed");

    if(errors) {
        std::cout << "FAIL" << std::endl;
        return(2);
    }
    else {
        std::cout << "OK" << std::endl;
        return(0);
    }
}
