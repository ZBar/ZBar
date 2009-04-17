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

// NB do not put anything before this header
// it's here to check that we didn't omit any dependencies
#include <zbar.h>

int main (int argc, char **argv)
{
    const char *video_dev = "/dev/video0";
    if(argc > 1)
        video_dev = argv[1];

    zbar::Processor proc = zbar::Processor(true, video_dev);
    proc.set_visible();
    proc.set_active();
    try {
        proc.user_wait();
    }
    catch(zbar::ClosedError) { }
    
    return(0);
}
