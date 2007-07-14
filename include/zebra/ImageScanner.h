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
#ifndef _ZEBRA_IMAGE_SCANNER_H_
#define _ZEBRA_IMAGE_SCANNER_H_

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/ImageScanner.h"
#endif

#include "Symbol.h"

namespace zebra {

class ImageScanner {
 public:
    class RangeException : public std::exception {
        virtual const char* what () const throw()
        {
            return("index out of range");
        }
    };

    ImageScanner ()
    {
        _scanner = zebra_img_scanner_create();
    }

    ImageScanner (unsigned width, unsigned height)
    {
        _scanner = zebra_img_scanner_create();
        set_size(width, height);
    }

    ~ImageScanner ()
    {
        zebra_img_scanner_destroy(_scanner);
    }

    void set_size (unsigned width, unsigned height)
    {
        zebra_img_scanner_set_size(_scanner, width, height);
    }

    int scan_y (const void *image)
    {
        return(zebra_img_scan_y(_scanner, image));
    }

    int get_result_size ()
    {
        return(zebra_img_scanner_get_result_size(_scanner));
    }

    Symbol get_result (unsigned index)
    {
        zebra_symbol_t *sym = zebra_img_scanner_get_result(_scanner, index);
        if(sym)
            return(Symbol(sym));
        throw RangeException();
    }

    template <typename OutputIterator>
    OutputIterator result_iter (OutputIterator out)
    {
        for(int i = get_result_size(); i; i--)
            *out++ = get_result(i);
        return(out);
    }

 private:
    zebra_img_scanner_t *_scanner;
};

}

#endif
