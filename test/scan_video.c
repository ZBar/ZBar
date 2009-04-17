/*------------------------------------------------------------------------
 *  Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
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

#include <stdio.h>
#include <string.h>
#include <ftw.h>

typedef void (cb_t) (void *userdata, const char *device);

static cb_t *add_dev;
static void *userdata;
static int idx;
static const char *default_dev;
static int default_idx;


static int video_filter (const char *fpath,
                         const struct stat *sb,
                         int typeflag)
{
    if(S_ISCHR(sb->st_mode) && (sb->st_rdev >> 8) == 81 && fpath) {
        int active = default_dev && !strcmp(default_dev, fpath);
        if(strncmp(fpath, "/dev/", 5)) {
            char abs[strlen(fpath) + 6];
            strcpy(abs, "/dev/");
            if(fpath[0] == '/')
                abs[4] = '\0';
            strcat(abs, fpath);
            add_dev(userdata, abs);
            active |= default_dev && !strcmp(default_dev, abs);
        }
        else
            add_dev(userdata, fpath);

        if(active) {
            default_idx = idx + 1;
            default_dev = NULL;
        }
        idx++;
    }
    return(0);
}

/* scan /dev for v4l video devices and call add_device for each.
 * also looks for a specified "default" device (if not NULL)
 * if not found, the default will be appended to the list.
 * returns the index+1 of the default_device, or 0 if the default
 * was not specified.  NB *not* reentrant
 */
int scan_video (cb_t add_device,
                void *_userdata,
                const char *default_device)
{
    add_dev = add_device;
    userdata = _userdata;
    default_dev = default_device;
    idx = default_idx = 0;

    if(ftw("/dev", video_filter, 4)) {
        perror("search for video devices failed");
        default_dev = NULL;
        default_idx = -1;
    }

    if(default_dev) {
        /* default not found in list, add explicitly */
        add_dev(userdata, default_dev);
        default_idx = ++idx;
        default_dev = NULL;
    }

    add_dev = userdata = NULL;
    return(default_idx);
}
