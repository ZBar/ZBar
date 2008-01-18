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
    Exception (const char *msg = "zebra library unspecified generic error")
        : std::exception(),
          _msg(msg)
    { }

    ~Exception () throw() { }

    virtual const char* what () const throw()
    {
        return(_msg.c_str());
    }

protected:
    std::string _msg;
};

class InternalError : public Exception {
public:
    InternalError (const char *msg = "internal library error")
        : Exception(msg)
    { }
};

class UnsupportedError : public Exception {
public:
    UnsupportedError (const char *msg = "unsupported request")
        : Exception(msg)
    { }
};

class InvalidError : public Exception {
public:
    InvalidError (const char *msg = "invalid request")
        : Exception(msg)
    { }
};

class SystemError : public Exception {
public:
    SystemError (const char *msg = "system error")
        : Exception(msg)
    { }
};

class LockingError : public Exception {
public:
    LockingError (const char *msg = "locking error")
        : Exception(msg)
    { }
};

class BusyError : public Exception {
public:
    BusyError (const char *msg = "all resources busy")
        : Exception(msg)
    { }
};

class XDisplayError : public Exception {
public:
    XDisplayError (const char *msg = "X11 display error")
        : Exception(msg)
    { }
};

class XProtoError : public Exception {
public:
    XProtoError (const char *msg = "X11 protocol error")
        : Exception(msg)
    { }
};

class ClosedError : public Exception {
public:
    ClosedError (const char *msg = "output window is closed")
        : Exception(msg)
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

static inline std::exception create_exception (zebra_error_t code,
                                               const char *msg)
{
    switch(code) {
    case ZEBRA_ERR_NOMEM:
        return(std::bad_alloc());
    case ZEBRA_ERR_INTERNAL:
        return(InternalError(msg));
    case ZEBRA_ERR_UNSUPPORTED:
        return(UnsupportedError(msg));
    case ZEBRA_ERR_INVALID:
        return(InvalidError(msg));
    case ZEBRA_ERR_SYSTEM:
        return(SystemError(msg));
    case ZEBRA_ERR_LOCKING:
        return(LockingError(msg));
    case ZEBRA_ERR_BUSY:
        return(BusyError(msg));
    case ZEBRA_ERR_XDISPLAY:
        return(XDisplayError(msg));
    case ZEBRA_ERR_XPROTO:
        return(XProtoError(msg));
    case ZEBRA_ERR_CLOSED:
        return(ClosedError(msg));
    default:
        return(Exception(msg));
    }
}

}

#endif
