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
#ifndef _ZEBRA_VIDEO_H_
#define _ZEBRA_VIDEO_H_

/// @file
/// Video Input C++ wrapper

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Video.h"
#endif

#include "Image.h"

namespace zebra {

/// mid-level video source abstraction.
/// captures images from a video device

class Video {
public:
    /// constructor.
    Video (zebra_video_t *video = NULL)
    {
        if(video)
            _video = video;
        else
            _video = zebra_video_create();
    }

    /// constructor.
    Video (std::string& device)
    {
        _video = zebra_video_create();
        open(device);
    }

    ~Video ()
    {
        zebra_video_destroy(_video);
    }

    /// cast to C video object.
    operator zebra_video_t* () const
    {
        return(_video);
    }

    /// open and probe a video device.
    void open (std::string& device)
    {
        if(zebra_video_open(_video, device.c_str()))
            throw_exception(_video);
    }

    /// close video device if open.
    void close ()
    {
        if(zebra_video_open(_video, NULL))
            throw_exception(_video);
    }

    /// retrieve file descriptor associated with open *nix video device.
    /// see zebra_video_get_fd()
    int get_fd ()
    {
        return(zebra_video_get_fd(_video));
    }

    /// retrieve current output image width.
    /// see zebra_video_get_width()
    int get_width ()
    {
        return(zebra_video_get_width(_video));
    }

    /// retrieve current output image height.
    /// see zebra_video_get_height()
    int get_height ()
    {
        return(zebra_video_get_height(_video));
    }

    /// start/stop video capture.
    /// see zebra_video_enable()
    void enable (bool enable = true)
    {
        if(zebra_video_enable(_video, enable))
            throw_exception(_video);
    }

    /// retrieve next captured image.
    /// see zebra_video_next_image()
    Image next_image ()
    {
        zebra_image_t *img = zebra_video_next_image(_video);
        if(!img)
            throw_exception(_video);
        return(Image(img));
    }

private:
    zebra_video_t *_video;
};

}

#endif
