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
#ifndef _ZEBRA_IMAGE_H_
#define _ZEBRA_IMAGE_H_

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Image.h"
#endif

#include <assert.h>
#include <iterator>
#include "Symbol.h"
#include "Exception.h"

namespace zebra {

class Image {

public:
    class Handler {
    public:
        virtual ~Handler() { }
        virtual void image_callback(Image &image) = 0;

        // FIXME bogus
        operator zebra_image_data_handler_t* () const {
            return(_cb);
        }

    private:
        static void _cb (zebra_image_t *zimg,
                         const void *userdata)
        {
            if(userdata) {
                Image image(zimg);
                ((Handler*)userdata)->image_callback(image);
            }
        }
    };

    class SymbolIterator
        : public std::iterator<std::input_iterator_tag, Symbol> {

    public:
        SymbolIterator (const Image *img = NULL)
        {
            if(img) {
                const zebra_symbol_t *zsym = zebra_image_first_symbol(*img);
                _sym.init(zsym);
            }
        }

        SymbolIterator (const SymbolIterator& iter)
            : _sym(iter._sym)
        { }

        SymbolIterator& operator++ ()
        {
            const zebra_symbol_t *zsym = _sym;
            if(zsym) {
                zsym = zebra_symbol_next(zsym);
                _sym.init(zsym);
            }
            return(*this);
        }

        const Symbol& operator* () const
        {
            return(_sym);
        }

        bool operator== (const SymbolIterator& iter) const
        {
            return(_sym == iter._sym);
        }

        bool operator!= (const SymbolIterator& iter) const
        {
            return(!(*this == iter));
        }

    private:
        Symbol _sym;
    };


    Image (unsigned width = 0,
           unsigned height = 0,
           const std::string& format = "",
           const void *data = NULL,
           unsigned long length = 0)
        : _img(zebra_image_create())
    {
        if(width && height)
            set_size(width, height);
        if(format.length())
            set_format(format);
        if(data && length)
            set_data(data, length);
    }

    Image (zebra_image_t *src)
        : _img(src)
    { }

    ~Image ()
    {
        zebra_image_destroy(_img);
    }

    operator const zebra_image_t* () const
    {
        return(_img);
    }

    operator zebra_image_t* ()
    {
        return(_img);
    }


    unsigned long get_format () const
    {
        return(zebra_image_get_format(_img));
    }

    void set_format (unsigned long format)
    {
        zebra_image_set_format(_img, format);
    }

    void set_format (const std::string& format)
    {
        if(format.length() != 4)
            throw FormatError();
        unsigned long fourcc = ((format[0] & 0xff) |
                                ((format[1] & 0xff) << 8) |
                                ((format[2] & 0xff) << 16) |
                                ((format[3] & 0xff) << 24));
        zebra_image_set_format(_img, fourcc);
    }

    unsigned get_width () const
    {
        return(zebra_image_get_width(_img));
    }

    unsigned get_height () const
    {
        return(zebra_image_get_height(_img));
    }

    void set_size (unsigned width,
                   unsigned height)
    {
        zebra_image_set_size(_img, width, height);
    }

    const void *get_data () const
    {
        return(zebra_image_get_data(_img));
    }

    void set_data (const void *data,
                   unsigned long length)
    {
        zebra_image_set_data(_img, data, length, _cleanup);
    }

    Image convert (unsigned long format) const
    {
        zebra_image_t *img = zebra_image_convert(_img, format);
        if(img)
            return(Image(img));
        throw FormatError();
    }

    SymbolIterator symbol_begin() const {
        return(SymbolIterator(this));
    }

    SymbolIterator symbol_end() const {
        return(SymbolIterator(_sym_iter_end));
    }

protected:
    static void _cleanup (zebra_image_t *img)
    {
        // by default nothing is cleaned
        assert(img);
    }

private:
    zebra_image_t *_img;
    SymbolIterator _sym_iter_end;
};

}

#endif

