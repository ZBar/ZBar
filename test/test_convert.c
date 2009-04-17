/*------------------------------------------------------------------------
 *  Copyright 2007-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the ZBar Bar Code Reader.
 *
 *  The ZBar Bar Code Reader is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The ZBar Bar Code Reader is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the ZBar Bar Code Reader; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zbar
 *------------------------------------------------------------------------*/

#include <config.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <zbar.h>
#include "test_images.h"

#if 0
static uint32_t formats[] = {
    
};
#endif

int main (int argc, char *argv[])
{
    zbar_set_verbosity(10);

    uint32_t srcfmt = fourcc('I','4','2','0');
    if(argc > 1)
        srcfmt = *(uint32_t*)argv[1];

    zbar_image_t *img = zbar_image_create();
    zbar_image_set_size(img, 640, 480);
    zbar_image_set_format(img, srcfmt);
    if(test_image_bars(img))
        return(2);

    if(zbar_image_write(img, "base"))
        return(1);
    return(0);
}
