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
#ifndef _ZEBRA_SYMBOL_H_
#define _ZEBRA_SYMBOL_H_

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Symbol.h"
#endif

#include <string>
#include <ostream>

namespace zebra {

class Symbol {

public:
    class Point {
    public:
        int x, y;

        Point () { }

        Point (const Point& pt)
        {
            x = pt.x;
            y = pt.y;
        }
    };


    class PointIterator
        : public std::iterator<std::input_iterator_tag, Point> {

    public:
        PointIterator (const Symbol *sym = NULL,
                       int index = 0)
            : _sym(sym),
              _index(index)
        {
            if(!sym ||
               (unsigned)_index >= zebra_symbol_get_loc_size(*_sym))
                _index = -1;
        }

        PointIterator (const PointIterator& iter)
            : _sym(iter._sym),
              _index(iter._index)
        { }

        PointIterator& operator++ ()
        {
            if(_index >= 0) {
                _pt.x = zebra_symbol_get_loc_x(*_sym, ++_index);
                _pt.y = zebra_symbol_get_loc_y(*_sym, _index);
                if(_pt.x < 0 || _pt.y < 0)
                    _index = -1;
            }
            return(*this);
        }

        const Point& operator* () const
        {
            return(_pt);
        }

        bool operator== (const PointIterator& iter) const
        {
            return(_index == iter._index &&
                   ((_index < 0) || _sym == iter._sym));
        }

        bool operator!= (const PointIterator& iter) const
        {
            return(!(*this == iter));
        }

    private:
        const Symbol *_sym;
        int _index;
        Point _pt;
    };


    Symbol (const zebra_symbol_t *sym = NULL)
    {
        init(sym);
    }

    ~Symbol () { }


    void init (const zebra_symbol_t *sym)
    {
        _sym = sym;
        if(sym) {
            _type = zebra_symbol_get_type(sym);
            _data = zebra_symbol_get_data(sym);
        }
        else {
            _type = ZEBRA_NONE;
            _data = "";
        }
    }

    operator const zebra_symbol_t* () const
    {
        return(_sym);
    }

    bool operator== (const Symbol& sym) const
    {
        return(_sym == sym._sym);
    }

    bool operator!= (const Symbol& sym) const
    {
        return(!(*this == sym));
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


    PointIterator point_begin() const
    {
        return(PointIterator(this));
    }

    const PointIterator& point_end() const
    {
        return(_point_iter_end);
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
    const zebra_symbol_t *_sym;
    zebra_symbol_type_t _type;
    std::string _data;
    PointIterator _point_iter_end;
};

std::ostream& operator<< (std::ostream& out,
                          const Symbol& sym)
{
    out << sym.get_type_name()
        << sym.get_addon_name()
        << ":" << sym.get_data();
    return(out);
}

}

#endif
