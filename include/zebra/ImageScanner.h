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

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/ImageScanner.h"
#endif

#include "Image.h"

namespace zebra {

class ImageScanner {

public:
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

    operator zebra_image_scanner_t* () const
    {
        return(_scanner);
    }

    void set_handler (Image::Handler &handler)
    {
        zebra_image_scanner_set_data_handler(_scanner, handler, &handler);
    }

    int scan (Image& image)
    {
        return(zebra_scan_image(_scanner, image));
    }

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
