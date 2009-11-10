//------------------------------------------------------------------------
//  Copyright 2009 (c) Jeff Brown <spadix@users.sourceforge.net>
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

#include <sys/times.h>

#ifdef DEBUG_OBJC
# define zlog(fmt, ...) \
    NSLog(@"ZBarReaderController: " fmt , ##__VA_ARGS__)

#define timer_start \
    long t_start = timer_now();

static inline long timer_now ()
{
    struct tms now;
    times(&now);
    return(now.tms_utime + now.tms_stime);
}

static inline double timer_elapsed (long start, long end)
{
    double clk_tck = sysconf(_SC_CLK_TCK);
    if(!clk_tck)
        return(-1);
    return((end - start) / clk_tck);
}

#else
# define zlog(...)
# define timer_start
# define timer_now(...) 0
# define timer_elapsed(...) 0
#endif
