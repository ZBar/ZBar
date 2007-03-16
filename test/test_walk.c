/*------------------------------------------------------------------------
 *  Copyright 2007 (c) Jeff Brown <spadix@users.sourceforge.net>
 *
 *  This file is part of the Zebra Barcode Library.
 *
 *  The Zebra Barcode Library is free software; you can redistribute it
 *  and/or modify it under the terms of the GNU Lesser Public License as
 *  published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  The Zebra Barcode Library is distributed in the hope that it will be
 *  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 *  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser Public License
 *  along with the Zebra Barcode Library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 *  Boston, MA  02110-1301  USA
 *
 *  http://sourceforge.net/projects/zebra
 *------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>

#include <zebra.h>

static unsigned w, h, dx, dy;
static zebra_img_walker_t *walker;

static char test_handler (zebra_img_walker_t *walker,
                          void *p)
{
    unsigned x = zebra_img_walker_get_col(walker);
    if(x >= w) {
        fprintf(stderr, "ERROR: x=%x >= w=%x\n", x, w);
        return(-1);
    }
    unsigned y = zebra_img_walker_get_row(walker);
    if(y >= h) {
        fprintf(stderr, "ERROR: y=%x >= h=%x\n", y, h);
        return(-1);
    }
    unsigned long t = x * dx + y * dy;
    if((unsigned long)p != t) {
        fprintf(stderr, "ERROR: %x * %x + %x * %x = %06lx != %06lx\n",
                x, dx, y, dy, t, (unsigned long)p);
        return(-1);
    }
    return(0);
}

int main (int argc, char *argv[])
{
    if(argc < 3) {
        fprintf(stderr, "usage: test_walk <width> <height>"
                " [<bytes_per_col> [<bytes_per_row>]]\n");
        return(1);
    }

    walker = zebra_img_walker_create();

    w = strtol(argv[1], NULL, 0);
    h = strtol(argv[2], NULL, 0);
    zebra_img_walker_set_size(walker, w, h);

    dx = 1;
    if(argc >= 4)
        dx = strtol(argv[3], NULL, 0);
    dy = dx * w;
    if(argc >= 5)
        dy = strtol(argv[4], NULL, 0);
    zebra_img_walker_set_stride(walker, dx, dy);
    zebra_img_walker_set_handler(walker, test_handler);

    if(zebra_img_walk(walker, 0)) {
        fprintf(stderr, "consistency error!\n");
        return(2);
    }

    return(0);
}
