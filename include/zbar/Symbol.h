//------------------------------------------------------------------------
//  Copyright 2007-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
//
//  This file is part of the ZBar Bar Code Reader.
//
//  The ZBar Bar Code Reader is free software; you can redistribute it
//  and/or modify it under the terms of the GNU Lesser Public License as
//  published by the Free Software Foundation; either version 2.1 of
//  the License, or (at your option) any later version.
//
//  The ZBar Bar Code Reader is distributed in the hope that it will be
//  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
//  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser Public License for more details.
//
//  You should have received a copy of the GNU Lesser Public License
//  along with the ZBar Bar Code Reader; if not, write to the Free
//  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
//  Boston, MA  02110-1301  USA
//
//  http://sourceforge.net/projects/zbar
//------------------------------------------------------------------------
#ifndef _ZBAR_SYMBOL_H_
#define _ZBAR_SYMBOL_H_

/// @file
/// Symbol C++ wrapper

#ifndef _ZBAR_H_
# error "include zbar.h in your application, **not** zbar/Symbol.h"
#endif

#include <string>
#include <ostream>

namespace zbar {

/// decoded barcode symbol result object.  stores type, data, and
/// image location of decoded symbol

class Symbol {
public:

    /// image pixel location (x, y) coordinate tuple.
    class Point {
    public:
        int x;  ///< x-coordinate.
        int y;  ///< y-coordinate.

        Point () { }

        /// copy constructor.
        Point (const Point& pt)
        {
            x = pt.x;
            y = pt.y;
        }
    };

    /// iteration over Point objects in a symbol location polygon.
    class PointIterator
        : public std::iterator<std::input_iterator_tag, Point> {

    public:
        /// constructor.
        PointIterator (const Symbol *sym = NULL,
                       int index = 0)
            : _sym(sym),
              _index(index)
        {
            if(!sym ||
               (unsigned)_index >= zbar_symbol_get_loc_size(*_sym))
                _index = -1;
        }

        /// constructor.
        PointIterator (const PointIterator& iter)
            : _sym(iter._sym),
              _index(iter._index)
        { }

        /// advance iterator to next Point.
        PointIterator& operator++ ()
        {
            if(_index >= 0) {
                _pt.x = zbar_symbol_get_loc_x(*_sym, ++_index);
                _pt.y = zbar_symbol_get_loc_y(*_sym, _index);
                if(_pt.x < 0 || _pt.y < 0)
                    _index = -1;
            }
            return(*this);
        }

        /// retrieve currently referenced Point.
        const Point& operator* () const
        {
            return(_pt);
        }

        /// test if two iterators refer to the same Point in the same
        /// Symbol.
        bool operator== (const PointIterator& iter) const
        {
            return(_index == iter._index &&
                   ((_index < 0) || _sym == iter._sym));
        }

        /// test if two iterators refer to the same Point in the same
        /// Symbol.
        bool operator!= (const PointIterator& iter) const
        {
            return(!(*this == iter));
        }

    private:
        const Symbol *_sym;
        int _index;
        Point _pt;
    };

    /// constructor.
    Symbol (const zbar_symbol_t *sym = NULL)
        : _xmlbuf(NULL),
          _xmllen(0)
    {
        init(sym);
    }

    ~Symbol () {
        if(_xmlbuf)
            free(_xmlbuf);
    }

    /// initialize Symbol from C symbol object.
    void init (const zbar_symbol_t *sym)
    {
        _sym = sym;
        if(sym) {
            _type = zbar_symbol_get_type(sym);
            _data = zbar_symbol_get_data(sym);
            _count = zbar_symbol_get_count(sym);
        }
        else {
            _type = ZBAR_NONE;
            _data = "";
            _count = -1;
        }
    }

    /// cast to C symbol.
    operator const zbar_symbol_t* () const
    {
        return(_sym);
    }

    /// test if two Symbol objects refer to the same C symbol.
    bool operator== (const Symbol& sym) const
    {
        return(_sym == sym._sym);
    }

    /// test if two Symbol objects refer to the same C symbol.
    bool operator!= (const Symbol& sym) const
    {
        return(!(*this == sym));
    }

    /// retrieve type of decoded symbol.
    zbar_symbol_type_t get_type () const
    {
        return(_type);
    }

    /// retrieve the string name of the symbol type.
    const std::string get_type_name () const
    {
        return(zbar_get_symbol_name(_type));
    }

    /// retrieve the string name for any addon.
    const std::string get_addon_name () const
    {
        return(zbar_get_addon_name(_type));
    }

    /// retrieve ASCII data decoded from symbol.
    const std::string get_data () const
    {
        return(_data);
    }

    /// retrieve inter-frame coherency count.
    /// see zbar_symbol_get_count()
    /// @since 1.5
    int get_count () const
    {
        return(_count);
    }

    /// create a new PointIterator at the start of the location
    /// polygon.
    PointIterator point_begin() const
    {
        return(PointIterator(this));
    }

    /// return a PointIterator suitable for ending iteration.
    const PointIterator& point_end() const
    {
        return(_point_iter_end);
    }

    /// see zbar_symbol_get_loc_size().
    int get_location_size () const
    {
        return(zbar_symbol_get_loc_size(_sym));
    }

    /// see zbar_symbol_get_loc_x().
    int get_location_x (unsigned index) const
    {
        return(zbar_symbol_get_loc_x(_sym, index));
    }

    /// see zbar_symbol_get_loc_y().
    int get_location_y (unsigned index) const
    {
        return(zbar_symbol_get_loc_y(_sym, index));
    }

    /// see zbar_symbol_xml().
    const std::string xml () const
    {
        return(zbar_symbol_xml(_sym, (char**)&_xmlbuf, (unsigned*)&_xmllen));
    }

private:
    const zbar_symbol_t *_sym;
    zbar_symbol_type_t _type;
    std::string _data;
    int _count;
    PointIterator _point_iter_end;
    char *_xmlbuf;
    unsigned _xmllen;
};

/// @relates Symbol
/// stream the string representation of a Symbol.
static inline std::ostream& operator<< (std::ostream& out,
                                        const Symbol& sym)
{
    out << sym.get_type_name()
        << sym.get_addon_name()
        << ":" << sym.get_data();
    return(out);
}

}

#endif
