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
#ifndef _ZEBRA_SYMBOL_H_
#define _ZEBRA_SYMBOL_H_

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Symbol.h"
#endif

#include <string>

namespace zebra {

class Symbol {
 public:
    Symbol (zebra_symbol_t *sym)
        : _sym(sym),
          _type(zebra_symbol_get_type(sym)),
          _data(zebra_symbol_get_data(sym))
    {
    }

    ~Symbol ()
    {
    }

    zebra_symbol_type_t get_type () const
    {
        return(_type);
    }

    const std::string get_type_name () const
    {
        return(zebra_get_symbol_name(_type));
    }

    const std::string get_addon_name () const
    {
        return(zebra_get_addon_name(_type));
    }

    const std::string get_data () const
    {
        return(_data);
    }

    int get_location_size () const
    {
        return(zebra_symbol_get_loc_size(_sym));
    }

    int get_location_x (unsigned index) const
    {
        return(zebra_symbol_get_loc_x(_sym, index));
    }

    int get_location_y (unsigned index) const
    {
        return(zebra_symbol_get_loc_y(_sym, index));
    }

 private:
    zebra_symbol_t *_sym;
    zebra_symbol_type_t _type;
    std::string _data;
};

}

#endif
