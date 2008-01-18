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
#ifndef _ZEBRA_PROCESSOR_H_
#define _ZEBRA_PROCESSOR_H_

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Processor.h"
#endif

#include "Image.h"

namespace zebra {

class Processor {
 public:
    static const int FOREVER = -1;

    Processor (bool threaded = true)
    {
        _processor = zebra_processor_create(threaded);
    }

    Processor (bool threaded = true,
               const char *video_device = "",
               bool enable_display = true)
    {
        _processor = zebra_processor_create(threaded);
        init(video_device, enable_display);
    }

    ~Processor ()
    {
        zebra_processor_destroy(_processor);
    }

    operator zebra_processor_t* ()
    {
        return(_processor);
    }

    void init (const char *video_device = "",
               bool enable_display = true)
    {
        zebra_processor_init(_processor, video_device, enable_display);
    }

    void set_handler (Image::Handler& handler)
    {
        zebra_processor_set_data_handler(_processor, handler, &handler);
    }

    bool is_visible ()
    {
        return(zebra_processor_is_visible(_processor));
    }

    void set_visible (bool visible = true)
    {
        zebra_processor_set_visible(_processor, visible);
    }

    void set_active (bool active = true)
    {
        zebra_processor_set_active(_processor, active);
    }

    void user_wait (int timeout = FOREVER)
    {
        zebra_processor_user_wait(_processor, timeout);
    }

    void process_one (int timeout = FOREVER)
    {
        zebra_process_one(_processor, timeout);
    }

    void process_image (Image& image)
    {
        zebra_process_image(_processor, image);
    }

    Processor& operator<< (Image& image)
    {
        process_image(image);
        return(*this);
    }

 private:
    zebra_processor_t *_processor;
};

}

#endif
