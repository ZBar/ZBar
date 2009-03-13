//------------------------------------------------------------------------
//  Copyright 2007-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
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

/// @file
/// Processor C++ wrapper

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Processor.h"
#endif

#include "Exception.h"
#include "Image.h"

namespace zebra {

/// high-level self-contained image processor.
/// processes video and images for barcodes, optionally displaying
/// images to a library owned output window

class Processor {
 public:
    /// value to pass for no timeout.
    static const int FOREVER = -1;

    /// constructor.
    Processor (bool threaded = true,
               const char *video_device = "",
               bool enable_display = true)
    {
        _processor = zebra_processor_create(threaded);
        if(!_processor)
            throw std::bad_alloc();
        init(video_device, enable_display);
    }

    ~Processor ()
    {
        zebra_processor_destroy(_processor);
    }

    /// cast to C processor object.
    operator zebra_processor_t* ()
    {
        return(_processor);
    }

    /// opens a video input device and/or prepares to display output.
    /// see zebra_processor_init()
    void init (const char *video_device = "",
               bool enable_display = true)
    {
        if(zebra_processor_init(_processor, video_device, enable_display))
            throw_exception(_processor);
    }

    /// setup result handler callback.
    /// see zebra_processor_set_data_handler()
    void set_handler (Image::Handler& handler)
    {
        zebra_processor_set_data_handler(_processor, handler, &handler);
    }

    /// set config for indicated symbology (0 for all) to specified value.
    /// @see zebra_processor_set_config()
    /// @since 0.4
    int set_config (zebra_symbol_type_t symbology,
                    zebra_config_t config,
                    int value)
    {
        return(zebra_processor_set_config(_processor, symbology,
                                          config, value));
    }

    /// set config parsed from configuration string.
    /// @see zebra_processor_parse_config()
    /// @since 0.4
    int set_config (std::string cfgstr)
    {
        return(zebra_processor_parse_config(_processor, cfgstr.c_str()));
    }

    /// retrieve the current state of the ouput window.
    /// see zebra_processor_is_visible()
    bool is_visible ()
    {
        int rc = zebra_processor_is_visible(_processor);
        if(rc < 0)
            throw_exception(_processor);
        return(rc != 0);
    }

    /// show or hide the display window owned by the library.
    /// see zebra_processor_set_visible()
    void set_visible (bool visible = true)
    {
        if(zebra_processor_set_visible(_processor, visible) < 0)
            throw_exception(_processor);
    }

    /// control the processor in free running video mode.
    /// see zebra_processor_set_active()
    void set_active (bool active = true)
    {
        if(zebra_processor_set_active(_processor, active) < 0)
            throw_exception(_processor);
    }

    /// wait for input to the display window from the user.
    /// see zebra_processor_user_wait()
    int user_wait (int timeout = FOREVER)
    {
        int rc = zebra_processor_user_wait(_processor, timeout);
        if(rc < 0)
            throw_exception(_processor);
        return(rc);
    }

    /// process from the video stream until a result is available.
    /// see zebra_process_one()
    void process_one (int timeout = FOREVER)
    {
        if(zebra_process_one(_processor, timeout) < 0)
            throw_exception(_processor);
    }

    /// process the provided image for barcodes.
    /// see zebra_process_image()
    void process_image (Image& image)
    {
        if(zebra_process_image(_processor, image) < 0)
            throw_exception(_processor);
    }

    /// process the provided image for barcodes.
    /// see zebra_process_image()
    Processor& operator<< (Image& image)
    {
        process_image(image);
        return(*this);
    }

    /// force specific input and output formats for debug/testing.
    /// see zebra_processor_force_format()
    void force_format (unsigned long input_format,
                       unsigned long output_format)
    {
        if(zebra_processor_force_format(_processor, input_format,
                                        output_format))
            throw_exception(_processor);
    }

    /// force specific input and output formats for debug/testing.
    /// see zebra_processor_force_format()
    void force_format (std::string& input_format,
                       std::string& output_format)
    {
        unsigned long ifourcc = *(unsigned long*)input_format.c_str();
        unsigned long ofourcc = *(unsigned long*)output_format.c_str();
        if(zebra_processor_force_format(_processor, ifourcc, ofourcc))
            throw_exception(_processor);
    }

    /// request a preferred size for the video image from the device.
    /// see zebra_processor_request_size()
    /// @since 0.6
    void request_size (int width, int height)
    {
        zebra_processor_request_size(_processor, width, height);
    }

    /// request a preferred driver interface version for debug/testing.
    /// see zebra_processor_request_interface()
    /// @since 0.6
    void request_interface (int version)
    {
        zebra_processor_request_interface(_processor, version);
    }

    /// request a preferred I/O mode for debug/testing.
    /// see zebra_processor_request_iomode()
    /// @since 0.7
    void request_iomode (int iomode)
    {
        if(zebra_processor_request_iomode(_processor, iomode))
            throw_exception(_processor);
    }

 private:
    zebra_processor_t *_processor;
};

}

#endif
