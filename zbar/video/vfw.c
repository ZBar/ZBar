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

#include "video.h"
#include <vfw.h>

#include <assert.h>

#define MAX_DRIVERS 10
#define MAX_NAME 128

#define BIH_FMT "%ldx%ld @%dbpp (%lx) cmp=%.4s(%08lx) res=%ldx%ld clr=%ld/%ld (%lx)"
#define BIH_FIELDS(bih)                                                 \
    (bih)->biWidth, (bih)->biHeight, (bih)->biBitCount, (bih)->biSizeImage, \
        (char*)&(bih)->biCompression, (bih)->biCompression,             \
        (bih)->biXPelsPerMeter, (bih)->biYPelsPerMeter,                 \
        (bih)->biClrImportant, (bih)->biClrUsed, (bih)->biSize

static const uint32_t vfw_formats[] = {
    /* planar YUV formats */
    fourcc('I','4','2','0'),
    /* FIXME YU12 is IYUV is windows */
    fourcc('Y','V','1','2'),
    /* FIXME IMC[1-4]? */

    /* planar Y + packed UV plane */
    fourcc('N','V','1','2'),

    /* packed YUV formats */
    fourcc('U','Y','V','Y'),
    fourcc('Y','U','Y','2'), /* FIXME add YVYU */
    /* FIXME AYUV? Y411? Y41P? */

    /* packed rgb formats */
    fourcc('B','G','R','3'),
    fourcc('B','G','R','4'),

    fourcc('Y','V','U','9'),

    /* basic grayscale format */
    fourcc('G','R','E','Y'),
    fourcc('Y','8','0','0'),

    /* compressed formats */
    fourcc('J','P','E','G'),

    /* terminator */
    0
};

#define VFW_NUM_FORMATS (sizeof(vfw_formats) / sizeof(uint32_t))

/* this seems to be required so the message pump can run, even when
 * we're not capturing...anyway, what's one more thread in the huge
 * fabric that is vfw?  :|
 */
#define ZBAR_VFW_THREADED 1

#ifdef ZBAR_VFW_THREADED
static DWORD WINAPI vfw_capture_thread (void *arg)
{
    zbar_video_t *vdo = arg;

    vdo->hwnd = capCreateCaptureWindow(NULL, WS_POPUP, 0, 0, 1, 1, NULL, 0);

    /* FIXME error */
    assert(vdo->hwnd);

    SetEvent(vdo->notify);
    zprintf(4, "spawned vfw capture thread\n");

    MSG msg;
    int rc = -2;
    while((rc = GetMessage(&msg, NULL, 0, 0)) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if(vdo->hwnd) {
        DestroyWindow(vdo->hwnd);
        vdo->hwnd = NULL;
    }
    zprintf(4, "exiting vfw capture thread\n");
    return(rc);
}
#endif

static LRESULT CALLBACK vfw_stream_cb (HWND hwnd,
                                       VIDEOHDR *hdr)
{
    if(!hwnd || !hdr)
        return(0);
    zbar_video_t *vdo = (void*)capGetUserData(hwnd);

    video_lock(vdo);
    zbar_image_t *img = vdo->image;
    if(!img) {
        video_lock(vdo);
        img = video_dq_image(vdo);
    }
    if(img) {
        img->data = hdr->lpData;
        img->datalen = hdr->dwBufferLength;
        vdo->image = img;
        SetEvent(vdo->notify);
    }
    video_unlock(vdo);

    zprintf(64, "img=%p\n", vdo->image);
    return(1);
}

static LRESULT CALLBACK vfw_control_cb (HWND hwnd,
                                        int state)
{
    if(!hwnd)
        return(0);
    zbar_video_t *vdo = (void*)capGetUserData(hwnd);
    zprintf(64, "thr=%04lx vdo=%p state=%d\n",
            GetCurrentThreadId(), vdo, state);

#if 0
    switch(state) {
    case CONTROLCALLBACK_PREROLL:
        break;
    case CONTROLCALLBACK_CAPTURING:
        break;
    }
#endif
    return(1);
}

static LRESULT CALLBACK vfw_error_cb (HWND hwnd,
                                      int errid,
                                      const char *errmsg)
{
    if(!hwnd)
        return(0);
    zbar_video_t *vdo = (void*)capGetUserData(hwnd);
    zprintf(2, "thr=%04lx vdo=%p id=%d msg=%s\n",
            GetCurrentThreadId(), vdo, errid, errmsg);
    return(1);
}

static int vfw_nq (zbar_video_t *vdo,
                   zbar_image_t *img)
{
    zprintf(64,"thr=%04lx vdo=%p img=%p\n", GetCurrentThreadId(), vdo, img);
    img->data = NULL;
    img->datalen = 0;
    return(video_nq_image(vdo, img));
}

static zbar_image_t *vfw_dq (zbar_video_t *vdo)
{
    zbar_image_t *img = vdo->image;
    zprintf(64,"thr=%04lx vdo=%p img=%p\n", GetCurrentThreadId(), vdo, img);
    if(!img) {
        video_unlock(vdo);

#ifdef ZBAR_VFW_THREADED
        WaitForSingleObject(vdo->notify, INFINITE);
#else
        int rc = 1;
        while(rc == 1) {
            MSG msg;
            rc = MsgWaitForMultipleObjects(1, &vdo->notify,
                                           0, INFINITE, QS_ALLINPUT);
            if(rc == 1) {
                GetMessage(&msg, NULL, 0, 0);
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
#endif
        video_lock(vdo);
        img = vdo->image;
        /*FIXME errors */
    }
    if(img)
        vdo->image = NULL;
    video_unlock(vdo);

    zprintf(64,"img=%p\n", img);
    return(img);
}

static int vfw_start (zbar_video_t *vdo)
{
    zprintf(64,"enter...\n");
    if(!capCaptureSequenceNoFile(vdo->hwnd))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "starting video stream"));
    zprintf(64,"...leave\n");
    return(0);
}

static int vfw_stop (zbar_video_t *vdo)
{
    zprintf(64,"\n");
    if(!capCaptureAbort(vdo->hwnd))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "stopping video stream"));
    return(0);
}

static int vfw_set_format (zbar_video_t *vdo,
                           uint32_t fmt)
{
    const zbar_format_def_t *fmtdef = _zbar_format_lookup(fmt);
    if(!fmtdef->format)
        return(err_capture_int(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                               "unsupported vfw format: %x", fmt));

    BITMAPINFOHEADER *bih = vdo->bih;
    assert(vdo->bih);
    bih->biWidth = vdo->width;
    bih->biHeight = vdo->height;
    switch(fmtdef->group) {
    case ZBAR_FMT_GRAY:
        bih->biBitCount = 8;
        break;
    case ZBAR_FMT_YUV_PLANAR:
    case ZBAR_FMT_YUV_PACKED:
    case ZBAR_FMT_YUV_NV:
        bih->biBitCount = 8 + (16 >> (fmtdef->p.yuv.xsub2 + fmtdef->p.yuv.ysub2));
        break;
    case ZBAR_FMT_RGB_PACKED:
        bih->biBitCount = fmtdef->p.rgb.bpp * 8;
        break;
    default:
        bih->biBitCount = 0;
    }
    bih->biClrUsed = bih->biClrImportant = 0;
    bih->biCompression = fmt;

    zprintf(4, "seting format: %.4s(%08x) " BIH_FMT "\n",
            (char*)&fmt, fmt, BIH_FIELDS(bih));

    if(!capSetVideoFormat(vdo->hwnd, bih, vdo->bi_size))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                           "setting format"));

    if(!capGetVideoFormat(vdo->hwnd, bih, vdo->bi_size))
        return(-1/*FIXME*/);

    if(bih->biCompression != fmt)
        return(-1/*FIXME*/);

    vdo->format = fmt;
    vdo->width = bih->biWidth;
    vdo->height = bih->biHeight;
    vdo->datalen = bih->biSizeImage;

    zprintf(4, "set new format: %.4s(%08x) " BIH_FMT "\n",
            (char*)&fmt, fmt, BIH_FIELDS(bih));
    return(0);
}

static int vfw_init (zbar_video_t *vdo,
                     uint32_t fmt)
{
    if(vfw_set_format(vdo, fmt))
        return(-1);

    CAPTUREPARMS cp;
    if(!capCaptureGetSetup(vdo->hwnd, &cp, sizeof(cp)))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "retrieving capture parameters"));

    cp.dwRequestMicroSecPerFrame = 33333;
    cp.fMakeUserHitOKToCapture = 0;
    cp.wPercentDropForError = 90;
    cp.fYield = 1;
    cp.wNumVideoRequested = vdo->num_images;
    cp.fCaptureAudio = 0;
    cp.vKeyAbort = 0;
    cp.fAbortLeftMouse = 0;
    cp.fAbortRightMouse = 0;
    cp.fLimitEnabled = 0;

    if(!capCaptureSetSetup(vdo->hwnd, &cp, sizeof(cp)))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "setting capture parameters"));

    if(!capCaptureGetSetup(vdo->hwnd, &cp, sizeof(cp)))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_WINAPI, __func__,
                           "checking capture parameters"));

    /* ignore errors since we skipped checking fHasOverlay */
    capOverlay(vdo->hwnd, 0);

    if(!capPreview(vdo->hwnd, 0) ||
       !capPreviewScale(vdo->hwnd, 0))
        err_capture(vdo, SEV_WARNING, ZBAR_ERR_WINAPI, __func__,
                    "disabling preview");

    capSetCallbackOnCapControl(vdo->hwnd, vfw_control_cb);

    if(!capSetCallbackOnVideoStream(vdo->hwnd, vfw_stream_cb) ||
       !capSetCallbackOnError(vdo->hwnd, vfw_error_cb))
        return(err_capture(vdo, SEV_ERROR, ZBAR_ERR_BUSY, __func__,
                           "setting capture callbacks"));

    vdo->num_images = cp.wNumVideoRequested;
    vdo->iomode = VIDEO_MMAP; /* driver provides "locked" buffers */

    zprintf(3, "initialized video capture: %d buffers %ldms/frame\n",
            vdo->num_images, cp.dwRequestMicroSecPerFrame);

    return(0);
}

static int vfw_probe_format (zbar_video_t *vdo,
                             uint32_t fmt)
{
    const zbar_format_def_t *fmtdef = _zbar_format_lookup(fmt);
    if(!fmtdef)
        return(0);

    zprintf(4, "    trying %.4s(%08x)...\n", (char*)&fmt, fmt);
    BITMAPINFOHEADER *bih = vdo->bih;
    bih->biWidth = vdo->width;
    bih->biHeight = vdo->height;
    switch(fmtdef->group) {
    case ZBAR_FMT_GRAY:
        bih->biBitCount = 8;
        break;
    case ZBAR_FMT_YUV_PLANAR:
    case ZBAR_FMT_YUV_PACKED:
    case ZBAR_FMT_YUV_NV:
        bih->biBitCount = 8 + (16 >> (fmtdef->p.yuv.xsub2 + fmtdef->p.yuv.ysub2));
        break;
    case ZBAR_FMT_RGB_PACKED:
        bih->biBitCount = fmtdef->p.rgb.bpp * 8;
        break;
    default:
        bih->biBitCount = 0;
    }
    bih->biCompression = fmt;

    if(!capSetVideoFormat(vdo->hwnd, bih, vdo->bi_size)) {
        zprintf(4, "\tno (set fails)\n");
        return(0);
    }

    if(!capGetVideoFormat(vdo->hwnd, bih, vdo->bi_size))
        return(0/*FIXME error...*/);

    zprintf(6, "\tactual: " BIH_FMT "\n", BIH_FIELDS(bih));

    if(bih->biCompression != fmt) {
        zprintf(4, "\tno (set ignored)\n");
        return(0);
    }

    zprintf(4, "\tyes\n");
    return(1);
}

static int vfw_probe (zbar_video_t *vdo)
{
    if(!capSetUserData(vdo->hwnd, (LONG)vdo))
        return(-1/*FIXME*/);

    vdo->bi_size = capGetVideoFormatSize(vdo->hwnd);
    if(!vdo->bi_size)
        return(-1/*FIXME*/);

    BITMAPINFOHEADER *bih = vdo->bih = realloc(vdo->bih, vdo->bi_size);
    /* FIXME check OOM */
    if(!capGetVideoFormat(vdo->hwnd, bih, vdo->bi_size))
        return(-1/*FIXME*/);

    zprintf(3, "initial format: " BIH_FMT " (bisz=%x)\n",
            BIH_FIELDS(bih), vdo->bi_size);

    if(!vdo->width || !vdo->height) {
        vdo->width = bih->biWidth;
        vdo->height = bih->biHeight;
    }
    vdo->datalen = bih->biSizeImage;

    zprintf(2, "probing supported formats:\n");
    vdo->formats = calloc(VFW_NUM_FORMATS, sizeof(uint32_t));

    int n = 0;
    const uint32_t *fmt;
    for(fmt = vfw_formats; *fmt; fmt++)
        if(vfw_probe_format(vdo, *fmt))
            vdo->formats[n++] = *fmt;

    vdo->formats = realloc(vdo->formats, (n + 1) * sizeof(uint32_t));

    vdo->width = bih->biWidth;
    vdo->height = bih->biHeight;
    vdo->intf = VIDEO_VFW;
    vdo->init = vfw_init;
    vdo->start = vfw_start;
    vdo->stop = vfw_stop;
    vdo->cleanup = vfw_stop;
    vdo->nq = vfw_nq;
    vdo->dq = vfw_dq;
    return(0);
}

int _zbar_video_open (zbar_video_t *vdo,
                      const char *dev)
{
    /* close open device */
    if(vdo->hwnd) {
        video_lock(vdo);
        if(vdo->active) {
            vdo->active = 0;
            vdo->stop(vdo);
        }
        if(vdo->cleanup)
            vdo->cleanup(vdo);

        SendMessage(vdo->hwnd, WM_QUIT, 0, 0);
        /* FIXME wait for thread to terminate */
        /*assert(!vdo->hwnd);*/

        zprintf(1, "closed camera\n");
        vdo->hwnd = NULL;
        vdo->intf = VIDEO_INVALID;
        vdo->fd = -1;
        video_unlock(vdo);
    }
    if(!dev)
        return(0);

    int devid;
    if(*(unsigned char*)dev < 0x10)
        devid = *dev;
    else if((!strncmp(dev, "/dev/video", 10) ||
             !strncmp(dev, "\\dev\\video", 10)) &&
            dev[10] >= '0' && dev[10] <= '9' && !dev[11])
        devid = dev[10] - '0';
    else
        devid = -1;

    zprintf(6, "searching for camera: %s (%d)\n", dev, devid);
    char name[MAX_NAME], desc[MAX_NAME];
    for(vdo->fd = 0; vdo->fd < MAX_DRIVERS; vdo->fd++) {
        if(!capGetDriverDescription(vdo->fd, name, MAX_NAME, desc, MAX_NAME)) {
            /* FIXME TBD error */
            zprintf(6, "    [%d] not found...\n", vdo->fd);
            continue;
        }
        zprintf(6, "    [%d] %.100s - %.100s\n", vdo->fd, name, desc);
        if((devid >= 0)
           ? vdo->fd == devid
           : !strncmp(dev, name, MAX_NAME))
            break;
    }
    if(vdo->fd >= MAX_DRIVERS)
        return(err_capture_str(vdo, SEV_ERROR, ZBAR_ERR_INVALID, __func__,
                               "video device not found '%s'", dev));

#ifdef ZBAR_VFW_THREADED
    HANDLE hthr = CreateThread(NULL, 0, vfw_capture_thread, vdo, 0, NULL);
    if(!hthr)
        return(-1/*FIXME*/);
    CloseHandle(hthr);

    if(WaitForSingleObject(vdo->notify, INFINITE))
        return(-1/*FIXME*/);
#else
    vdo->hwnd = capCreateCaptureWindow(NULL, WS_POPUP, 0, 0, 1, 1, NULL, 0);
#endif

    /* FIXME error */
    assert(vdo->hwnd);

    if(!capDriverConnect(vdo->hwnd, vdo->fd)) {
        DestroyWindow(vdo->hwnd);
        /* FIXME error: failed to connect to camera */
        assert(0);
    }

    zprintf(1, "opened camera: %.60s (thr=%04lx)\n",
            name, GetCurrentThreadId());

    if(vfw_probe(vdo)) {
        _zbar_video_open(vdo, NULL);
        return(-1);
    }
    return(0);
}
