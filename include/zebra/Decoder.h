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
#ifndef _ZEBRA_DECODER_H_
#define _ZEBRA_DECODER_H_

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Decoder.h"
#endif

#include <string>

namespace zebra {

class Decoder {
 public:
    class Handler {
    public:
        virtual ~Handler() { }
        virtual void decode_callback(Decoder &decoder) = 0;
    };

    Decoder ()
        : _handler(NULL)
    {
        _decoder = zebra_decoder_create();
    }

    ~Decoder ()
    {
        zebra_decoder_destroy(_decoder);
    }

    void reset ()
    {
        zebra_decoder_reset(_decoder);
    }

    void new_scan ()
    {
        zebra_decoder_new_scan(_decoder);
    }

    zebra_symbol_type_t decode_width (unsigned width)
    {
        return(zebra_decode_width(_decoder, width));
    }

    Decoder& operator<< (unsigned width)
    {
        zebra_decode_width(_decoder, width);
        return(*this);
    }

    zebra_color_t get_color () const
    {
        return(zebra_decoder_get_color(_decoder));
    }

    zebra_symbol_type_t get_type () const
    {
        return(zebra_decoder_get_type(_decoder));
    }

    const char *get_symbol_name () const
    {
        return(zebra_get_symbol_name(zebra_decoder_get_type(_decoder)));
    }

    const char *get_addon_name () const
    {
        return(zebra_get_addon_name(zebra_decoder_get_type(_decoder)));
    }

    const char *get_data_chars() const
    {
        return(zebra_decoder_get_data(_decoder));
    }

    const std::string& get_data_string() const
    {
        ((Decoder*)this)->_data = zebra_decoder_get_data(_decoder);
        return(_data);
    }

    void set_handler (Handler &handler)
    {
        _handler = &handler;
        zebra_decoder_set_handler(_decoder, _cb);
        zebra_decoder_set_userdata(_decoder, this);
    }

 private:
    friend class Scanner;
    zebra_decoder_t *_decoder;
    std::string _data;
    Handler *_handler;

    static void _cb (zebra_decoder_t *cdcode)
    {
        Decoder *dcode = (Decoder*)zebra_decoder_get_userdata(cdcode);
        if(dcode && dcode->_handler)
            dcode->_handler->decode_callback(*dcode);
    }
};

}

#endif
