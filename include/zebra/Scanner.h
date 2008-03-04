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

/// @file
/// Scanner C++ wrapper

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Scanner.h"
#endif

#include <stdio.h>

namespace zebra {

/// low-level linear intensity sample stream scanner interface.
/// identifies "bar" edges and measures width between them.
/// optionally passes to bar width Decoder

class Scanner {
 public:

    /// constructor.
    /// @param decoder reference to a Decoder instance which will
    /// be passed scan results automatically
    Scanner (Decoder& decoder)
    {
        _scanner = zebra_scanner_create(decoder._decoder);
    }

    /// constructor.
    /// @param decoder pointer to a Decoder instance which will
    /// be passed scan results automatically
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

    /// clear all scanner state.
    /// see zebra_scanner_reset()
    void reset ()
    {
        zebra_scanner_reset(_scanner);
    }

    /// mark start of a new scan pass.
    /// see zebra_scanner_new_scan()
    void new_scan ()
    {
        zebra_scanner_new_scan(_scanner);
    }

    /// process next sample intensity value.
    /// see zebra_scan_y()
    zebra_symbol_type_t scan_y (int y)
    {
        _type = zebra_scan_y(_scanner, y);
        return(_type);
    }

    /// process next sample intensity value.
    /// see zebra_scan_y()
    Scanner& operator<< (int y)
    {
        _type = zebra_scan_y(_scanner, y);
        return(*this);
    }

    /// process next sample from RGB (or BGR) triple.
    /// see zebra_scan_rgb24()
    zebra_symbol_type_t scan_rgb24 (unsigned char *rgb)
    {
        _type = zebra_scan_rgb24(_scanner, rgb);
        return(_type);
    }

    /// process next sample from RGB (or BGR) triple.
    /// see zebra_scan_rgb24()
    Scanner& operator<< (unsigned char *rgb)
    {
        _type = zebra_scan_rgb24(_scanner, rgb);
        return(*this);
    }

    /// retrieve last scanned width.
    /// see zebra_scanner_get_width()
    unsigned get_width () const
    {
        return(zebra_scanner_get_width(_scanner));
    }

    /// retrieve last scanned color.
    /// see zebra_scanner_get_color()
    zebra_color_t get_color () const
    {
        return(zebra_scanner_get_color(_scanner));
    }

    /// retrieve last scan result.
    zebra_symbol_type_t get_type () const
    {
        return(_type);
    }

    /// cast to C scanner
    operator zebra_scanner_t* () const
    {
        return(_scanner);
    }

    /// retrieve C scanner
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
