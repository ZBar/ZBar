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
#ifndef _ZEBRA_IMAGE_WALKER_H_
#define _ZEBRA_IMAGE_WALKER_H_

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/ImageWalker.h"
#endif

namespace zebra {

class ImageWalker {
 public:
    class Handler {
    public:
        virtual ~Handler() { }
        virtual char walker_callback(ImageWalker &walker, void *pixel) = 0;
    };

    ImageWalker ()
        : _handler(NULL)
    {
        _walker = zebra_img_walker_create();
    }

    ~ImageWalker ()
    {
        zebra_img_walker_destroy(_walker);
    }

    void set_size (unsigned width, unsigned height)
    {
        zebra_img_walker_set_size(_walker, width, height);
    }

    void set_stride (unsigned bytes_per_col, unsigned bytes_per_row)
    {
        zebra_img_walker_set_stride(_walker, bytes_per_col, bytes_per_row);
    }

    char walk (const void *image)
    {
        return(zebra_img_walk(_walker, image));
    }

    unsigned get_col()
    {
        return(zebra_img_walker_get_col(_walker));
    }

    unsigned get_row()
    {
        return(zebra_img_walker_get_row(_walker));
    }

    void set_handler (Handler &handler)
    {
        _handler = &handler;
        zebra_img_walker_set_handler(_walker, _cb);
        zebra_img_walker_set_userdata(_walker, this);
    }

 private:
    zebra_img_walker_t *_walker;
    Handler *_handler;

    static char _cb (zebra_img_walker_t *cwalk, void *pixel)
    {
        ImageWalker *walk = (ImageWalker*)zebra_img_walker_get_userdata(cwalk);
        if(walk && walk->_handler)
            return(walk->_handler->walker_callback(*walk, pixel));
        return(0);
    }
};

}

#endif
