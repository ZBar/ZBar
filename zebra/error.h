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
#ifndef _ERROR_H_
#define _ERROR_H_

#include <config.h>
#ifdef HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <zebra.h>

#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

#define ERRINFO_MAGIC (0x5252457a) /* "zERR" (LE) */

typedef enum errsev_e {
    SEV_FATAL   = -2,           /* application must terminate */
    SEV_ERROR   = -1,           /* might be able to recover and continue */
    SEV_OK      =  0,
    SEV_WARNING =  1,           /* unexpected condition */
    SEV_NOTE    =  2,           /* fyi */
} errsev_t;

typedef enum errmodule_e {
    ZEBRA_MOD_PROCESSOR,
    ZEBRA_MOD_VIDEO,
    ZEBRA_MOD_WINDOW,
    ZEBRA_MOD_IMAGE_SCANNER,
    ZEBRA_MOD_UNKNOWN,
} errmodule_t;

typedef struct errinfo_s {
    uint32_t magic;             /* just in case */
    errmodule_t module;         /* reporting module */
    char *buf;                  /* formatted and passed to application */
    int errnum;                 /* errno for system errors */

    errsev_t sev;
    zebra_error_t type;
    const char *func;           /* reporting function */
    const char *detail;         /* description */
    char *arg_str;              /* single string argument */
    int arg_int;                /* single integer argument */
} errinfo_t;

extern int _zebra_verbosity;

#define zprintf(level, format, ...)                                     \
    if(_zebra_verbosity >= level)                                       \
        fprintf(stderr, "%s: " format, __func__ , ##__VA_ARGS__)

static inline int err_copy (void *dst_c,
                            void *src_c)
{
    errinfo_t *dst = dst_c;
    errinfo_t *src = src_c;
    assert(dst->magic == ERRINFO_MAGIC);
    assert(src->magic == ERRINFO_MAGIC);

    dst->errnum = src->errnum;
    dst->sev = src->sev;
    dst->type = src->type;
    dst->func = src->func;
    dst->detail = src->detail;
    dst->arg_str = src->arg_str;
    dst->arg_int = src->arg_int;
    return(-1);
}

static inline int err_capture (const void *container,
                               errsev_t sev,
                               zebra_error_t type,
                               const char *func,
                               const char *detail)
{
    errinfo_t *err = (errinfo_t*)container;
    assert(err->magic == ERRINFO_MAGIC);
    if(type == ZEBRA_ERR_SYSTEM)
        err->errnum = errno;
    err->sev = sev;
    err->type = type;
    err->func = func;
    err->detail = detail;
    if(_zebra_verbosity >= 1)
        _zebra_error_spew(err, 0);
    return(-1);
}

static inline int err_capture_str (const void *container,
                                   errsev_t sev,
                                   zebra_error_t type,
                                   const char *func,
                                   const char *detail,
                                   const char *arg)
{
    err_capture(container, sev, type, func, detail);
    errinfo_t *err = (errinfo_t*)container;
    if(err->arg_str)
        free(err->arg_str);
    err->arg_str = strdup(arg);
    return(-1);
}

static inline int err_capture_int (const void *container,
                                   errsev_t sev,
                                   zebra_error_t type,
                                   const char *func,
                                   const char *detail,
                                   int arg)
{
    err_capture(container, sev, type, func, detail);
    ((errinfo_t*)container)->arg_int = arg;
    return(-1);
}

static inline int err_capture_num (const void *container,
                                   errsev_t sev,
                                   zebra_error_t type,
                                   const char *func,
                                   const char *detail,
                                   int num)
{
    err_capture(container, sev, type, func, detail);
    ((errinfo_t*)container)->errnum = num;
    return(-1);
}

static inline void err_init (errinfo_t *err,
                             errmodule_t module)
{
    err->magic = ERRINFO_MAGIC;
    err->module = module;
}

static inline void err_cleanup (errinfo_t *err)
{
    assert(err->magic == ERRINFO_MAGIC);
    if(err->buf) {
        free(err->buf);
        err->buf = NULL;
    }
}

#endif
