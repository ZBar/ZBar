/*------------------------------------------------------------------------
 *  Copyright 2007-2010 (c) Jeff Brown <spadix@users.sourceforge.net>
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
#ifndef _TEST_IMAGES_H_
#define _TEST_IMAGES_H_

#define fourcc zbar_fourcc

#ifdef __cplusplus

extern "C" {
    int test_image_check_cleanup(void);
    int test_image_bars(zbar::zbar_image_t*);
    int test_image_ean13(zbar::zbar_image_t*);
}

#else

int test_image_check_cleanup(void);
int test_image_bars(zbar_image_t*);
int test_image_ean13(zbar_image_t*);

#endif

extern const char *test_image_ean13_data;

#endif
