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

#include "processor.h"
#include <assert.h>

/* lock is already held */
int _zbar_process_image (zbar_processor_t *proc,
                         zbar_image_t *img)
{
    if(img) {
        uint32_t format = zbar_image_get_format(img);
        zprintf(16, "processing: %.4s(%08" PRIx32 ") %dx%d @%p\n",
                (char*)&format, format,
                zbar_image_get_width(img), zbar_image_get_height(img),
                zbar_image_get_data(img));

        /* FIXME locking all other interfaces while processing is conservative
         * but easier for now and we don't expect this to take long...
         */
        int nsyms = zbar_scan_image(proc->scanner, img);
        if(nsyms < 0)
            return(err_capture(proc, SEV_ERROR, ZBAR_ERR_UNSUPPORTED,
                               __func__, "unknown image format"));

        if(_zbar_verbosity >= 8) {
            const zbar_symbol_t *sym = zbar_image_first_symbol(img);
            while(sym) {
                zbar_symbol_type_t type = zbar_symbol_get_type(sym);
                int count = zbar_symbol_get_count(sym);
                zprintf(8, "%s%s: %s (%s)\n",
                        zbar_get_symbol_name(type),
                        zbar_get_addon_name(type),
                        zbar_symbol_get_data(sym),
                        (count < 0) ? "uncertain" :
                        (count > 0) ? "duplicate" : "new");
                sym = zbar_symbol_next(sym);
            }
        }

        if(nsyms) {
            _zbar_processor_notify(proc, EVENT_OUTPUT);
            if(proc->handler)
                /* FIXME only call after filtering */
                proc->handler(img, proc->userdata);
        }

        if(proc->force_output) {
            img = zbar_image_convert(img, proc->force_output);
            if(!img)
                return(err_capture(proc, SEV_ERROR, ZBAR_ERR_UNSUPPORTED,
                                   __func__, "unknown image format"));
        }
    }

    /* display to window if enabled */
    if(proc->window &&
       (zbar_window_draw(proc->window, img) ||
        _zbar_processor_invalidate(proc)))
        return(err_copy(proc, proc->window));

    if(proc->force_output && img)
        zbar_image_destroy(img);
    return(0);
}

zbar_processor_t *zbar_processor_create (int threaded)
{
    zbar_processor_t *proc = calloc(1, sizeof(zbar_processor_t));
    if(!proc)
        return(NULL);
    err_init(&proc->err, ZBAR_MOD_PROCESSOR);

    proc->scanner = zbar_image_scanner_create();
    if(!proc->scanner) {
        free(proc);
        return(NULL);
    }

    _zbar_processor_threads_init(proc, threaded);

    return(proc);
}

void zbar_processor_destroy (zbar_processor_t *proc)
{
    _zbar_processor_lock(proc);

    zbar_processor_init(proc, NULL, 0);

    if(proc->scanner) {
        zbar_image_scanner_destroy(proc->scanner);
        proc->scanner = NULL;
    }

    _zbar_processor_unlock(proc);

    _zbar_processor_threads_cleanup(proc);

    err_cleanup(&proc->err);

    free(proc);
}

int zbar_processor_init (zbar_processor_t *proc,
                         const char *dev,
                         int enable_display)
{
    if(_zbar_processor_lock(proc))
        return(-1);

    _zbar_processor_threads_stop(proc);

    _zbar_processor_close(proc);

    if(proc->window) {
        zbar_window_destroy(proc->window);
        proc->window = NULL;
    }

    int rc = 0;
    if(proc->video) {
        if(dev)
            zbar_video_open(proc->video, NULL);
        else {
            zbar_video_destroy(proc->video);
            proc->video = NULL;
        }
    }

    if(!dev && !enable_display)
        /* nothing to do */
        goto done;

    if(enable_display) {
        proc->window = zbar_window_create();
        if(!proc->window) {
            rc = err_capture(proc, SEV_FATAL, ZBAR_ERR_NOMEM,
                             __func__, "allocating window resources");
            goto done;
        }
    }

    if(dev) {
        proc->video = zbar_video_create();
        if(!proc->video) {
            rc = err_capture(proc, SEV_FATAL, ZBAR_ERR_NOMEM,
                             __func__, "allocating video resources");
            goto done;
        }
        if(proc->req_width || proc->req_height)
            zbar_video_request_size(proc->video,
                                     proc->req_width, proc->req_height);
        if(proc->req_intf)
            zbar_video_request_interface(proc->video, proc->req_intf);
        if(proc->req_iomode &&
           zbar_video_request_iomode(proc->video, proc->req_iomode)) {
            rc = err_copy(proc, proc->video);
            goto done;
        }
    }

    rc = _zbar_processor_threads_start(proc, dev);
    if(rc)
        goto done;

    if(proc->window) {
        /* arbitrary default */
        int width = 640, height = 480;
        if(proc->video) {
            width = zbar_video_get_width(proc->video);
            height = zbar_video_get_height(proc->video);
        }

        if(_zbar_processor_open(proc, "zbar barcode reader", width, height)) {
            rc = -1;
            goto done;
        }
    }

    if(proc->video && proc->force_input) {
        if(zbar_video_init(proc->video, proc->force_input))
            rc = err_copy(proc, proc->video);
    }
    else if(proc->video) {
        int retry = -1;
        if(proc->window) {
            retry = zbar_negotiate_format(proc->video, proc->window);
            if(retry)
                fprintf(stderr,
                        "WARNING: no compatible input to output format\n"
                        "...trying again with output disabled\n");
        }
        if(retry)
            retry = zbar_negotiate_format(proc->video, NULL);

        if(retry) {
            zprintf(1, "ERROR: no compatible %s format\n",
                    (proc->video) ? "video input" : "window output");
            rc = err_capture(proc, SEV_ERROR, ZBAR_ERR_UNSUPPORTED,
                             __func__, "no compatible image format");
        }
    }

 done:
    _zbar_processor_unlock(proc);
    return(rc);
}

zbar_image_data_handler_t*
zbar_processor_set_data_handler (zbar_processor_t *proc,
                                 zbar_image_data_handler_t *handler,
                                 const void *userdata)
{
    if(_zbar_processor_lock(proc) < 0)
        return(NULL);

    zbar_image_data_handler_t *result = proc->handler;
    proc->handler = handler;
    proc->userdata = userdata;

    _zbar_processor_unlock(proc);
    return(result);
}

void zbar_processor_set_userdata (zbar_processor_t *proc,
                                  void *userdata)
{
    if(_zbar_processor_lock(proc) < 0)
        return;

    proc->userdata = userdata;

    _zbar_processor_unlock(proc);
}

void *zbar_processor_get_userdata (const zbar_processor_t *proc)
{
    _zbar_processor_lock(proc);

    void *userdata = (void*)proc->userdata;

    _zbar_processor_unlock(proc);
    return(userdata);
}

int zbar_processor_set_config (zbar_processor_t *proc,
                               zbar_symbol_type_t sym,
                               zbar_config_t cfg,
                               int val)
{
    if(_zbar_processor_lock(proc) < 0)
        return(-1);
    int rc = zbar_image_scanner_set_config(proc->scanner, sym, cfg, val);
    _zbar_processor_unlock(proc);
    return(rc);
}

int zbar_processor_request_size (zbar_processor_t *proc,
                                 unsigned width,
                                 unsigned height)
{
    if(_zbar_processor_lock(proc) < 0)
        return(-1);

    proc->req_width = width;
    proc->req_height = height;

    _zbar_processor_unlock(proc);
    return(0);
}

int zbar_processor_request_interface (zbar_processor_t *proc,
                                      int ver)
{
    if(_zbar_processor_lock(proc) < 0)
        return(-1);

    proc->req_intf = ver;

    _zbar_processor_unlock(proc);
    return(0);
}

int zbar_processor_request_iomode (zbar_processor_t *proc,
                                   int iomode)
{
    if(_zbar_processor_lock(proc) < 0)
        return(-1);

    proc->req_iomode = iomode;

    _zbar_processor_unlock(proc);
    return(0);
}

int zbar_processor_force_format (zbar_processor_t *proc,
                                 unsigned long input,
                                 unsigned long output)
{
    if(_zbar_processor_lock(proc) < 0)
        return(-1);

    proc->force_input = input;
    proc->force_output = output;

    _zbar_processor_unlock(proc);
    return(0);
}

int zbar_processor_is_visible (zbar_processor_t *proc)
{
    if(_zbar_processor_lock(proc) < 0)
        return(-1);

    int visible = proc->window && proc->visible;

    _zbar_processor_unlock(proc);
    return(visible);
}

int zbar_processor_set_visible (zbar_processor_t *proc,
                                int visible)
{
    if(_zbar_processor_lock(proc) < 0)
        return(-1);

    int rc = 0;
    if(proc->window) {
        if(proc->video)
            rc = _zbar_processor_set_size(proc,
                                          zbar_video_get_width(proc->video),
                                          zbar_video_get_height(proc->video));
        if(!rc)
            rc = _zbar_processor_set_visible(proc, visible);
    }
    else if(visible)
        rc = err_capture(proc, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                         "processor display window not initialized");

    _zbar_processor_unlock(proc);
    return(rc);
}

int zbar_processor_user_wait (zbar_processor_t *proc,
                              int timeout)
{
    if(_zbar_processor_lock(proc) < 0)
        return(-1);

    int rc = -1;
    if(proc->visible || proc->active || timeout > 0)
        rc = _zbar_processor_wait(proc, EVENT_INPUT, timeout);
    if(rc > 0)
        rc = proc->input;

    if(!proc->visible)
        rc = err_capture(proc, SEV_WARNING, ZBAR_ERR_CLOSED, __func__,
                         "display window not available for input");

    _zbar_processor_unlock(proc);
    return(rc);
}

int zbar_processor_set_active (zbar_processor_t *proc,
                               int active)
{
    if(_zbar_processor_lock(proc) < 0)
        return(-1);

    if(!proc->video) {
        _zbar_processor_unlock(proc);
        return(err_capture(proc, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "video input not initialized"));
    }

    zbar_image_scanner_enable_cache(proc->scanner, active);
    int rc = zbar_video_enable(proc->video, active);
    if(!rc) {
        _zbar_processor_enable(proc, active);
        proc->active = active;
        proc->events &= ~EVENT_INPUT;
    }
    else
        err_copy(proc, proc->video);

    if(!proc->active && proc->window &&
       (zbar_window_draw(proc->window, NULL) ||
        _zbar_processor_invalidate(proc)) && !rc)
        rc = err_copy(proc, proc->window);

    _zbar_processor_notify(proc, 0);
    _zbar_processor_unlock(proc);
    return(rc);
}

int zbar_process_one (zbar_processor_t *proc,
                      int timeout)
{
    if(_zbar_processor_lock(proc) < 0)
        return(-1);

    int rc = 0;
    if(!proc->video) {
        rc = err_capture(proc, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                         "video input not initialized");
        goto done;
    }

    rc = zbar_processor_set_active(proc, 1);
    if(rc)
        goto done;

    rc = _zbar_processor_wait(proc, EVENT_OUTPUT, timeout);

    if(zbar_processor_set_active(proc, 0))
        rc = -1;

 done:
    _zbar_processor_unlock(proc);
    return(rc);
}

int zbar_process_image (zbar_processor_t *proc,
                        zbar_image_t *img)
{
    if(_zbar_processor_lock(proc) < 0)
        return(-1);

    int rc = 0;
    if(img && proc->window)
        rc = _zbar_processor_set_size(proc,
                                      zbar_image_get_width(img),
                                      zbar_image_get_height(img));
    if(!rc) {
        zbar_image_scanner_enable_cache(proc->scanner, 0);
        rc = _zbar_process_image(proc, img);
        if(proc->active)
            zbar_image_scanner_enable_cache(proc->scanner, 1);
    }

    _zbar_processor_unlock(proc);
    return(rc);
}
