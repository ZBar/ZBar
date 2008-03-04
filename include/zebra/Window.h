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
#ifndef _ZEBRA_WINDOW_H_
#define _ZEBRA_WINDOW_H_

/// @file
/// Output Window C++ wrapper

#ifndef _ZEBRA_H_
# error "include zebra.h in your application, **not** zebra/Window.h"
#endif

#include "Image.h"

namespace zebra {

/// mid-level output window abstraction.
/// displays images to user-specified platform specific output window

class Window {
public:
    /// constructor.
    Window (zebra_window_t *window = NULL)
    {
        if(window)
            _window = window;
        else
            _window = zebra_window_create();
    }

    /// constructor.
    Window (void *x11_display_w32_hwnd,
            unsigned long x11_drawable)
    {
        _window = zebra_window_create();
        attach(x11_display_w32_hwnd, x11_drawable);
    }

    ~Window ()
    {
        zebra_window_destroy(_window);
    }

    /// cast to C window object.
    operator zebra_window_t* () const
    {
        return(_window);
    }

    /// associate reader with an existing platform window.
    /// see zebra_window_attach()
    void attach (void *x11_display_w32_hwnd,
                 unsigned long x11_drawable)
    {
        if(zebra_window_attach(_window,
                               x11_display_w32_hwnd, x11_drawable) < 0)
            throw_exception(_window);
    }

    /// control content level of the reader overlay.
    /// see zebra_window_set_overlay()
    void set_overlay (int level)
    {
        zebra_window_set_overlay(_window, level);
    }

    /// draw a new image into the output window.
    /// see zebra_window_draw()
    void draw (Image& image)
    {
        if(zebra_window_draw(_window, image) < 0)
            throw_exception(_window);
    }

    /// redraw the last image.
    /// zebra_window_redraw()
    void redraw ()
    {
        if(zebra_window_redraw(_window) < 0)
            throw_exception(_window);
    }

    /// resize the image window.
    /// zebra_window_resize()
    void resize (unsigned width, unsigned height)
    {
        if(zebra_window_resize(_window, width, height) < 0)
            throw_exception(_window);
    }

private:
    zebra_window_t *_window;
};

/// select a compatible format between video input and output window.
/// see zebra_negotiate_format()
void negotiate_format (Video& video, Window &window)
{
    if(zebra_negotiate_format(video, window) < 0)
        throw_exception(video);
}

}

#endif
