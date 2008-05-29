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
#ifndef _ZEBRA_DECODER_H_
#define _ZEBRA_DECODER_H_

/// @file
/// Decoder C++ wrapper

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Decoder.h"
#endif

#include <string>

namespace zebra {

/// low-level bar width stream decoder interface.
/// identifies symbols and extracts encoded data

class Decoder {
 public:

    /// Decoder result handler.
    /// applications should subtype this and pass an instance to
    /// set_handler() to implement result processing
    class Handler {
    public:
        virtual ~Handler() { }

        /// invoked by the Decoder as decode results become available.
        virtual void decode_callback(Decoder &decoder) = 0;
    };

    /// constructor.
    Decoder ()
        : _handler(NULL)
    {
        _decoder = zebra_decoder_create();
    }

    ~Decoder ()
    {
        zebra_decoder_destroy(_decoder);
    }

    /// clear all decoder state.
    /// see zebra_decoder_reset()
    void reset ()
    {
        zebra_decoder_reset(_decoder);
    }

    /// mark start of a new scan pass.
    /// see zebra_decoder_new_scan()
    void new_scan ()
    {
        zebra_decoder_new_scan(_decoder);
    }

    /// process next bar/space width from input stream.
    /// see zebra_decode_width()
    zebra_symbol_type_t decode_width (unsigned width)
    {
        return(zebra_decode_width(_decoder, width));
    }

    /// process next bar/space width from input stream.
    /// see zebra_decode_width()
    Decoder& operator<< (unsigned width)
    {
        zebra_decode_width(_decoder, width);
        return(*this);
    }

    /// retrieve color of @em next element passed to Decoder.
    /// see zebra_decoder_get_color()
    zebra_color_t get_color () const
    {
        return(zebra_decoder_get_color(_decoder));
    }

    /// retrieve last decoded symbol type.
    /// see zebra_decoder_get_type()
    zebra_symbol_type_t get_type () const
    {
        return(zebra_decoder_get_type(_decoder));
    }

    /// retrieve string name of last decoded symbol type.
    /// see zebra_get_symbol_name()
    const char *get_symbol_name () const
    {
        return(zebra_get_symbol_name(zebra_decoder_get_type(_decoder)));
    }

    /// retrieve string name for last decode addon.
    /// see zebra_get_addon_name()
    const char *get_addon_name () const
    {
        return(zebra_get_addon_name(zebra_decoder_get_type(_decoder)));
    }

    /// retrieve last decoded data in ASCII format as a char array.
    /// see zebra_decoder_get_data()
    const char *get_data_chars() const
    {
        return(zebra_decoder_get_data(_decoder));
    }

    /// retrieve last decoded data in ASCII format as a std::string.
    /// see zebra_decoder_get_data()
    const std::string get_data_string() const
    {
        return(zebra_decoder_get_data(_decoder));
    }

    /// retrieve last decoded data in ASCII format as a std::string.
    /// see zebra_decoder_get_data()
    const std::string get_data() const
    {
        return(zebra_decoder_get_data(_decoder));
    }

    /// setup callback to handle result data.
    void set_handler (Handler &handler)
    {
        _handler = &handler;
        zebra_decoder_set_handler(_decoder, _cb);
        zebra_decoder_set_userdata(_decoder, this);
    }

    /// set config for indicated symbology (0 for all) to specified value.
    /// @see zebra_decoder_set_config()
    int set_config (zebra_symbol_type_t symbology,
                    zebra_config_t config,
                    int value)
    {
        return(zebra_decoder_set_config(_decoder, symbology, config, value));
    }

    /// set config parsed from configuration string.
    /// @see zebra_decoder_parse_config()
    int set_config (std::string cfgstr)
    {
        return(zebra_decoder_parse_config(_decoder, cfgstr.c_str()));
    }

 private:
    friend class Scanner;
    zebra_decoder_t *_decoder;
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
