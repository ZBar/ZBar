/*------------------------------------------------------------------------
 *  Copyright 2007-2008 (c) Jeff Brown <spadix@users.sourceforge.net>
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

/* varargs variations on compile time debug spew */

#ifndef DEBUG_LEVEL

# ifdef __GNUC__
    /* older versions of gcc (< 2.95) require a named varargs parameter */
#  define dprintf(args...)
# else
    /* unfortunately named vararg parameter is a gcc-specific extension */
#  define dprintf(...)
# endif

#else

# include <stdio.h>

# ifdef __GNUC__
#  define dprintf(level, args...) \
    if((level) <= DEBUG_LEVEL)    \
        fprintf(stderr, args)
# else
#  define dprintf(level, ...)     \
    if((level) <= DEBUG_LEVEL)    \
        fprintf(stderr, __VA_ARGS__)
# endif

#endif /* DEBUG_LEVEL */

/* spew warnings for non-fatal assertions.
 * returns specified error code if assertion fails.
 * NB check/return is still performed for NDEBUG
 * only the message is inhibited
 * FIXME don't we need varargs hacks here?
 */
#ifndef NDEBUG

# define zassert(condition, retval, format, ...) do {                   \
        if(!(condition)) {                                              \
            /*            fprintf(stderr, "WARNING: __file__:__line__: __func__: Assertion \"%s\" failed.\n\t" format , ##__VA_ARGS__);*/ \
            return(retval);                                             \
        }                                                               \
    } while(0)

#else

# define zassert(condition, retval, format, ...) do {   \
        if(!(condition))                                \
            return(retval);                             \
    } while(0)

#endif
