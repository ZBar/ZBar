/*------------------------------------------------------------------------
 *  Copyright 2009 (c) Jeff Brown <spadix@users.sourceforge.net>
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
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <zbar.h>

#include "test_images.h"

unsigned char jpeg[405] = {
    255, 216, 255, 224,   0,  16,  74,  70,
     73,  70,   0,   1,   1,   1,   0,  72,
      0,  72,   0,   0, 255, 219,   0,  67,
      0,   1,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   1,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      1,   1,   1,   1,   1,   1,   1,   1,
      1, 255, 219,   0,  67,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1,   1,   1,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      1,   1,   1,   1,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   1, 255, 192,
      0,  17,   8,   0,   8,   0,   8,   3,
      1,  17,   0,   2,  17,   1,   3,  17,
      1, 255, 196,   0,  20,   0,   1,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   7, 255,
    196,   0,  32,  16,   0,   1,   2,   5,
      5,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,  21,  20,  22,   0,
      8,  18,  19,  24,   6,  23,  36,  37,
     39, 255, 196,   0,  20,   1,   1,   0,
      0,   0,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,   0,   0,   9, 255,
    196,   0,  35,  17,   0,   2,   1,   1,
      7,   5,   0,   0,   0,   0,   0,   0,
      0,   0,   0,   0,  20,  21,  19,  22,
      0,   1,   7,  18,  23,  36,  38,   3,
      4,  35,  37,  52, 255, 218,   0,  12,
      3,   1,   0,   2,  17,   3,  17,   0,
     63,   0, 118,  93,  56,  89, 200, 157,
     68, 199, 111, 134,  71,  23,  12, 215,
    215, 130, 197, 136, 103, 143, 117, 170,
     97,  48,  42, 244, 202,  12, 216, 179,
    211, 183,  29, 252,  24,  42, 160, 197,
     45,  65, 146,  62, 181,  91,  48, 134,
     52, 246,  76, 170, 151,   4,  42, 137,
    198, 104,  56, 214,  96, 193,   7, 120,
    197,  15, 154, 194, 128, 216, 207, 170,
    114, 197, 220, 215,  36, 130, 123, 155,
    219, 184, 172, 222, 150, 146,  23, 191,
     47,  17, 204,   2, 197, 155, 246, 180,
    206, 226, 223, 255, 217,
};

unsigned char rgb[8*8*3] = {
    255, 255, 255,  176, 238, 176,   94, 220,  94,   60, 213,  60,
     60, 213,  60,   94, 220,  94,  176, 238, 176,  255, 255, 255,
    176, 238, 176,   46, 210,  46,   10, 102,  10,   17, 204,  17,
     17, 204,  17,   10, 102,  10,   46, 210,  46,  176, 238, 176,
     94, 220,  94,   19, 204,  19,    9, 102,   9,   17, 204,  17,
     17, 204,  17,    9, 102,   9,   19, 204,  19,   94, 220,  94,
     60, 213,  60,   17, 204,  17,    9, 102,   9,   17, 204,  17,
     17, 204,  17,    9, 102,   9,   17, 204,  17,   60, 213,  60,
     60, 213,  60,   17, 204,  17,   17, 204,  17,   17, 204,  17,
     17, 204,  17,   17, 204,  17,   17, 204,  17,   60, 213,  60,
     94, 220,  94,   10, 102,  10,   17, 204,  17,   17, 204,  17,
     17, 204,  17,   17, 204,  17,   10, 102,  10,   94, 220,  94,
    176, 238, 176,   46, 210,  46,   10, 102,  10,    9, 102,   9,
      9, 102,   9,   10, 102,  10,   46, 210,  46,  176, 238, 176,
    255, 255, 255,  176, 238, 176,   94, 220,  94,   60, 213,  60,
     60, 213,  60,   94, 220,  94,  176, 238, 176,  255, 255, 255,
};

int main (int argc, char **argv)
{
    zbar_set_verbosity(32);

    zbar_processor_t *proc = zbar_processor_create(0);
    assert(proc);
    if(zbar_processor_init(proc, NULL, 1))
        return(2);

    zbar_image_t *img = zbar_image_create();
    zbar_image_set_size(img, 8, 8);
    zbar_image_set_format(img, fourcc('J','P','E','G'));
    zbar_image_set_data(img, jpeg, sizeof(jpeg), NULL);

    zbar_image_t *test = zbar_image_convert(img, fourcc('Y','8','0','0'));
    if(!test)
        return(2);
    printf("converted: %d x %d (%lx) %08lx\n",
           zbar_image_get_width(test),
           zbar_image_get_height(test),
           zbar_image_get_data_length(test),
           zbar_image_get_format(test));

    if(zbar_process_image(proc, test) < 0)
        return(3);
    if(zbar_processor_set_visible(proc, 1))
        return(4);

    zbar_processor_user_wait(proc, -1);
    return(0);
}
