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
#ifndef _ZEBRA_EXCEPTION_H_
#define _ZEBRA_EXCEPTION_H_

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Exception.h"
#endif

namespace zebra {

class Exception : public std::exception {

public:
    Exception (const void *obj = NULL)
        : std::exception(),
          _obj(obj)
    { }

    ~Exception () throw() { }

    virtual const char* what () const throw()
    {
        if(!_obj)
            return("zebra library unspecified generic error");
        return(_zebra_error_string(_obj, 0));
    }

protected:
    const void *_obj;
};

class InternalError : public Exception {
public:
    InternalError (const void *obj)
        : Exception(obj)
    { }
};

class UnsupportedError : public Exception {
public:
    UnsupportedError (const void *obj)
        : Exception(obj)
    { }
};

class InvalidError : public Exception {
public:
    InvalidError (const void *obj)
        : Exception(obj)
    { }
};

class SystemError : public Exception {
public:
    SystemError (const void *obj)
        : Exception(obj)
    { }
};

class LockingError : public Exception {
public:
    LockingError (const void *obj)
        : Exception(obj)
    { }
};

class BusyError : public Exception {
public:
    BusyError (const void *obj)
        : Exception(obj)
    { }
};

class XDisplayError : public Exception {
public:
    XDisplayError (const void *obj)
        : Exception(obj)
    { }
};

class XProtoError : public Exception {
public:
    XProtoError (const void *obj)
        : Exception(obj)
    { }
};

class ClosedError : public Exception {
public:
    ClosedError (const void *obj)
        : Exception(obj)
    { }
};


// FIXME needs c equivalent
class FormatError : public Exception {
    virtual const char* what () const throw()
    {
        // FIXME what format?
        return("unsupported format");
    }
};

static inline std::exception throw_exception (const void *obj)
{
    switch(_zebra_get_error_code(obj)) {
    case ZEBRA_ERR_NOMEM:
        throw std::bad_alloc();
    case ZEBRA_ERR_INTERNAL:
        throw InternalError(obj);
    case ZEBRA_ERR_UNSUPPORTED:
        throw UnsupportedError(obj);
    case ZEBRA_ERR_INVALID:
        throw InvalidError(obj);
    case ZEBRA_ERR_SYSTEM:
        throw SystemError(obj);
    case ZEBRA_ERR_LOCKING:
        throw LockingError(obj);
    case ZEBRA_ERR_BUSY:
        throw BusyError(obj);
    case ZEBRA_ERR_XDISPLAY:
        throw XDisplayError(obj);
    case ZEBRA_ERR_XPROTO:
        throw XProtoError(obj);
    case ZEBRA_ERR_CLOSED:
        throw ClosedError(obj);
    default:
        throw Exception(obj);
    }
}

}

#endif
