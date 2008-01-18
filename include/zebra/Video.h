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

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Video.h"
#endif

#include "Image.h"

namespace zebra {

class Video {

public:
    Video (zebra_video_t *video = NULL)
    {
        if(video)
            _video = video;
        else
            _video = zebra_video_create();
    }

    Video (std::string& device)
    {
        _video = zebra_video_create();
        open(device);
    }

    ~Video ()
    {
        zebra_video_destroy(_video);
    }

    operator zebra_video_t* () const
    {
        return(_video);
    }

    std::exception get_error () const
    {
        return(create_exception(zebra_video_get_error_code(_video),
                                zebra_video_error_string(_video, 0)));
    }

    void open (std::string& device)
    {
        if(zebra_video_open(_video, device.c_str()))
            throw(get_error());
    }

    int get_fd ()
    {
        return(zebra_video_get_fd(_video));
    }

    int get_width ()
    {
        return(zebra_video_get_width(_video));
    }

    int get_height ()
    {
        return(zebra_video_get_height(_video));
    }

    void enable (bool enable = true)
    {
        if(zebra_video_enable(_video, enable))
            throw(get_error());
    }

    Image next_image ()
    {
        /* FIXME propagate image caching */
        zebra_image_t *img = zebra_video_next_image(_video);
        if(img)
            return(Image(img));
        throw(get_error());
    }

private:
    zebra_video_t *_video;
};

}

#endif
