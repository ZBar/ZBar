//------------------------------------------------------------------------
//  Copyright 2008 (c) Jeff Brown <spadix@users.sourceforge.net>
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
#ifndef _QZEBRAIMAGE_H_
#define _QZEBRAIMAGE_H_

/// @file
/// QImage to Image type conversion wrapper

#include <QImage>
#include <zebra.h>

namespace zebra {

/// wrap a QImage and convert into a format suitable for scanning.

class QZebraImage
    : public Image
{
public:

    /// construct a zebra library image based on an existing QImage.

    QZebraImage (const QImage &qimg)
        : qimg(qimg)
    {
        QImage::Format fmt = qimg.format();
        if(fmt != QImage::Format_RGB32 &&
           fmt != QImage::Format_ARGB32 &&
           fmt != QImage::Format_ARGB32_Premultiplied)
            throw FormatError();

        unsigned bpl = qimg.bytesPerLine();
        unsigned width = bpl / 4;
        unsigned height = qimg.height();
        set_size(width, height);
        set_format('B' | ('G' << 8) | ('R' << 16) | ('4' << 24));
        unsigned long datalen = qimg.numBytes();
        set_data(qimg.bits(), datalen);

        if((width * 4 != bpl) ||
           (width * height * 4 > datalen))
            throw FormatError();
    }

private:
    QImage qimg;
};

};


#endif
