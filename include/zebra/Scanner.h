//------------------------------------------------------------------------
//  Copyright 2007-2008 (c) Jeff Brown <spadix@users.sourceforge.net>
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
#ifndef _ZEBRA_SCANNER_H_
#define _ZEBRA_SCANNER_H_

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Scanner.h"
#endif

#include <stdio.h>

namespace zebra {

class Scanner {
 public:
    Scanner (Decoder& decoder)
    {
        _scanner = zebra_scanner_create(decoder._decoder);
    }

    Scanner (Decoder* decoder = NULL)
    {
        zebra_decoder_t *zdcode = NULL;
        if(decoder)
            zdcode = decoder->_decoder;
        _scanner = zebra_scanner_create(zdcode);
    }

    ~Scanner ()
    {
        zebra_scanner_destroy(_scanner);
    }

    void reset ()
    {
        zebra_scanner_reset(_scanner);
    }

    void new_scan ()
    {
        zebra_scanner_new_scan(_scanner);
    }

    zebra_symbol_type_t scan_y (int y)
    {
        _type = zebra_scan_y(_scanner, y);
        return(_type);
    }

    Scanner& operator<< (int y)
    {
        _type = zebra_scan_y(_scanner, y);
        return(*this);
    }

    zebra_symbol_type_t scan_rgb24 (unsigned char *rgb)
    {
        _type = zebra_scan_rgb24(_scanner, rgb);
        return(_type);
    }

    Scanner& operator<< (unsigned char *rgb)
    {
        _type = zebra_scan_rgb24(_scanner, rgb);
        return(*this);
    }

    unsigned get_width () const
    {
        return(zebra_scanner_get_width(_scanner));
    }

    zebra_color_t get_color () const
    {
        return(zebra_scanner_get_color(_scanner));
    }

    zebra_symbol_type_t get_type () const
    {
        return(_type);
    }

    operator zebra_scanner_t* () const
    {
        return(_scanner);
    }

    const zebra_scanner_t *get_c_scanner () const
    {
        return(_scanner);
    }

 private:
    zebra_scanner_t *_scanner;
    zebra_symbol_type_t _type;
};

}

#endif
