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
#ifndef _ZEBRA_IMAGE_SCANNER_H_
#define _ZEBRA_IMAGE_SCANNER_H_

/// @file
/// Image Scanner C++ wrapper

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/ImageScanner.h"
#endif

#include "Image.h"

namespace zebra {

/// mid-level image scanner interface.
/// reads barcodes from a 2-D Image

class ImageScanner {
public:
    /// constructor.
    ImageScanner (zebra_image_scanner_t *scanner = NULL)
    {
        if(scanner)
            _scanner = scanner;
        else
            _scanner = zebra_image_scanner_create();
    }

    ~ImageScanner ()
    {
        zebra_image_scanner_destroy(_scanner);
    }

    /// cast to C image_scanner object
    operator zebra_image_scanner_t* () const
    {
        return(_scanner);
    }

    /// setup result handler callback.
    void set_handler (Image::Handler &handler)
    {
        zebra_image_scanner_set_data_handler(_scanner, handler, &handler);
    }

    /// set config for indicated symbology (0 for all) to specified value.
    /// @see zebra_image_scanner_set_config()
    /// @since 0.4
    int set_config (zebra_symbol_type_t symbology,
                    zebra_config_t config,
                    int value)
    {
        return(zebra_image_scanner_set_config(_scanner, symbology,
                                              config, value));
    }

    /// set config parsed from configuration string.
    /// @see zebra_image_scanner_parse_config()
    /// @since 0.4
    int set_config (std::string cfgstr)
    {
        return(zebra_image_scanner_parse_config(_scanner, cfgstr.c_str()));
    }

    /// enable or disable the inter-image result cache.
    /// see zebra_image_scanner_enable_cache()
    void enable_cache (bool enable = true)
    {
        zebra_image_scanner_enable_cache(_scanner, enable);
    }

    /// scan for symbols in provided image.
    /// see zebra_scan_image()
    int scan (Image& image)
    {
        return(zebra_scan_image(_scanner, image));
    }

    /// scan for symbols in provided image.
    /// see zebra_scan_image()
    ImageScanner& operator<< (Image& image)
    {
        scan(image);
        return(*this);
    }

private:
    zebra_image_scanner_t *_scanner;
};

}

#endif
