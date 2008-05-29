/*------------------------------------------------------------------------
 *  Copyright 2008 (c) Jeff Brown <spadix@users.sourceforge.net>
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

#include <config.h>
#include <stdlib.h>     /* strtol */
#include <string.h>     /* strchr, strncmp, strlen */
#include <errno.h>
#include <assert.h>

#include <zebra.h>

int zebra_parse_config (const char *cfgstr,
                        zebra_symbol_type_t *sym,
                        zebra_config_t *cfg,
                        int *val)
{
    if(!cfgstr)
        return(1);

    const char *dot = strchr(cfgstr, '.');
    if(dot) {
        int symlen = dot - cfgstr;
        if(!symlen || !strncmp(cfgstr, "*", symlen))
            *sym = 0;
        else if(!strncmp(cfgstr, "ean8", symlen))
            *sym = ZEBRA_EAN8;
        else if(!strncmp(cfgstr, "isbn10", symlen))
            *sym = ZEBRA_ISBN10;
        else if(!strncmp(cfgstr, "upce", symlen))
            *sym = ZEBRA_UPCE;
        else if(!strncmp(cfgstr, "upca", symlen))
            *sym = ZEBRA_UPCA;
        else if(!strncmp(cfgstr, "ean13", symlen))
            *sym = ZEBRA_EAN13;
        else if(!strncmp(cfgstr, "isbn13", symlen))
            *sym = ZEBRA_ISBN13;
        else if(!strncmp(cfgstr, "i25", symlen))
            *sym = ZEBRA_I25;
        else if(!strncmp(cfgstr, "code39", symlen))
            *sym = ZEBRA_CODE39;
        else if(!strncmp(cfgstr, "code128", symlen))
            *sym = ZEBRA_CODE128;

#if 0
        /* FIXME addons are configured per-main symbol type */
        else if(!strncmp(cfgstr, "addon2", symlen))
            *sym = ZEBRA_ADDON2;
        else if(!strncmp(cfgstr, "addon5", symlen))
            *sym = ZEBRA_ADDON5;
#endif
        else
            return(1);
        cfgstr = dot + 1;
    }
    else
        *sym = 0;

    int cfglen = strlen(cfgstr);
    const char *eq = strchr(cfgstr, '=');
    if(eq)
        cfglen = eq - cfgstr;
    else
        *val = 1;  /* handle this here so we can override later */
    char negate = 0;

    if(cfglen > 3 && !strncmp(cfgstr, "no-", 3)) {
        negate = 1;
        cfgstr += 3;
        cfglen -= 3;
    }

    if(!strncmp(cfgstr, "enable", cfglen))
        *cfg = ZEBRA_CFG_ENABLE;
    else if(!strncmp(cfgstr, "disable", cfglen)) {
        *cfg = ZEBRA_CFG_ENABLE;
        negate = !negate; /* no-disable ?!? */
    }
    else if(!strncmp(cfgstr, "ascii", cfglen))
        *cfg = ZEBRA_CFG_ASCII;
    else if(!strncmp(cfgstr, "add-check", cfglen))
        *cfg = ZEBRA_CFG_ADD_CHECK;
    else if(!strncmp(cfgstr, "emit-check", cfglen))
        *cfg = ZEBRA_CFG_EMIT_CHECK;
    else
        return(1);

    if(eq) {
        errno = 0;
        *val = strtol(eq + 1, NULL, 0);
        if(errno)
            return(1);
    }
    if(negate)
        *val = !*val;

    return(0);
}
