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

#include <gtk/gtksignal.h>
#include <gdk/gdkx.h>

#include <zebra/zebragtk.h>
#include "zebragtkprivate.h"
#include "zebramarshal.h"

#ifndef G_PARAM_STATIC_STRINGS
# define G_PARAM_STATIC_STRINGS (G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB)
#endif

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

/* adapted from v4l2 spec */
#define fourcc(a, b, c, d)                      \
    ((long)(a) | ((long)(b) << 8) |             \
     ((long)(c) << 16) | ((long)(d) << 24))

GQuark zebra_gtk_error_quark ()
{
    return(g_quark_from_static_string("zebra_gtk_error"));
}

enum {
    DECODED,
    DECODED_TEXT,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_VIDEO_DEVICE,
    PROP_VIDEO_ENABLED,
    PROP_VIDEO_OPENED,
};

static guint zebra_gtk_signals[LAST_SIGNAL] = { 0 };

static gpointer zebra_gtk_parent_class = NULL;

/* FIXME what todo w/errors? OOM? */
/* FIXME signal failure notifications to main gui thread */

void zebra_gtk_release_pixbuf (zebra_image_t *img)
{
    GdkPixbuf *pixbuf = zebra_image_get_userdata(img);
    g_assert(GDK_IS_PIXBUF(pixbuf));

    /* remove reference */
    zebra_image_set_userdata(img, NULL);

    /* release reference to associated pixbuf and it's data */
    g_object_unref(pixbuf);
}

gboolean zebra_gtk_image_from_pixbuf (zebra_image_t *zimg,
                                      GdkPixbuf *pixbuf)
{
    /* apparently should always be packed RGB? */
    GdkColorspace colorspace = gdk_pixbuf_get_colorspace(pixbuf);
    if(colorspace != GDK_COLORSPACE_RGB) {
        g_warning("non-RGB color space not supported: %d\n", colorspace);
        return(FALSE);
    }

    int nchannels = gdk_pixbuf_get_n_channels(pixbuf);
    int bps = gdk_pixbuf_get_bits_per_sample(pixbuf);
    long type = 0;

    /* these are all guesses... */
    if(nchannels == 3 && bps == 8)
        type = fourcc('R','G','B','3');
    else if(nchannels == 4 && bps == 8)
        type = fourcc('B','G','R','4'); /* FIXME alpha flipped?! */
    else if(nchannels == 1 && bps == 8)
        type = fourcc('Y','8','0','0');
    else if(nchannels == 3 && bps == 5)
        type = fourcc('R','G','B','R');
    else if(nchannels == 3 && bps == 4)
        type = fourcc('R','4','4','4'); /* FIXME maybe? */
    else {
        g_warning("unsupported combination: nchannels=%d bps=%d\n",
                  nchannels, bps);
        return(FALSE);
    }
    zebra_image_set_format(zimg, type);

    /* FIXME we don't deal w/bpl...
     * this will cause problems w/unpadded pixbufs :|
     */
    unsigned pitch = gdk_pixbuf_get_rowstride(pixbuf);
    unsigned width = pitch / ((nchannels * bps) / 8);
    if((width * nchannels * 8 / bps) != pitch) {
        g_warning("unsupported: width=%d nchannels=%d bps=%d rowstride=%d\n",
                  width, nchannels, bps, pitch);
        return(FALSE);
    }
    unsigned height = gdk_pixbuf_get_height(pixbuf);
    /* FIXME this isn't correct either */
    unsigned long datalen = width * height * nchannels;
    zebra_image_set_size(zimg, width, height);

    /* when the zebra image is released, the pixbuf will be
     * automatically be released
     */
    zebra_image_set_userdata(zimg, pixbuf);
    zebra_image_set_data(zimg, gdk_pixbuf_get_pixels(pixbuf), datalen,
                         zebra_gtk_release_pixbuf);
#ifdef DEBUG_ZEBRAGTK
    g_message("colorspace=%d nchannels=%d bps=%d type=%.4s(%08lx)\n"
              "\tpitch=%d width=%d height=%d datalen=0x%lx\n",
              colorspace, nchannels, bps, (char*)&type, type,
              pitch, width, height, datalen);
#endif
    return(TRUE);
}

static inline gboolean zebra_gtk_video_open (ZebraGtk *self,
                                             const char *video_device)
{
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);
    gboolean video_opened = FALSE;

    gdk_threads_enter();

    zebra->req_width = DEFAULT_WIDTH;
    zebra->req_height = DEFAULT_HEIGHT;
    gtk_widget_queue_resize(GTK_WIDGET(self));

    zebra->video_opened = FALSE;
    if(zebra->thread)
        g_object_notify(G_OBJECT(self), "video-opened");

    if(zebra->window) {
        /* ensure old video doesn't have image ref
         * (FIXME handle video destroyed w/images outstanding)
         */
        zebra_window_draw(zebra->window, NULL);
        gtk_widget_queue_draw(GTK_WIDGET(self));
    }
    gdk_threads_leave();

    if(zebra->video) {
        zebra_video_destroy(zebra->video);
        zebra->video = NULL;
    }

    if(video_device && video_device[0] && zebra->thread) {
        /* create video
         * FIXME video should support re-open
         */
        zebra->video = zebra_video_create();
        g_assert(zebra->video);

        if(zebra_video_open(zebra->video, video_device)) {
            zebra_video_error_spew(zebra->video, 0);
            zebra_video_destroy(zebra->video);
            zebra->video = NULL;
            /* FIXME error propagation */
            return(FALSE);
        }

        /* negotiation accesses the window format list,
         * so we hold the lock for this part
         */
        gdk_threads_enter();

        video_opened = !zebra_negotiate_format(zebra->video, zebra->window);

        if(video_opened) {
            zebra->req_width = zebra_video_get_width(zebra->video);
            zebra->req_height = zebra_video_get_height(zebra->video);
        }
        gtk_widget_queue_resize(GTK_WIDGET(self));

        zebra->video_opened = video_opened;
        if(zebra->thread)
            g_object_notify(G_OBJECT(self), "video-opened");

        gdk_threads_leave();
    }
    return(video_opened);
}

static inline int zebra_gtk_process_image (ZebraGtk *self,
                                           zebra_image_t *image)
{
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    if(!image)
        return(-1);

    int rc = zebra_scan_image(zebra->scanner, image);
    if(rc < 0)
        return(rc);

    gdk_threads_enter();

    if(rc && zebra->thread) {
        /* update decode results */
        const zebra_symbol_t *sym;
        for(sym = zebra_image_first_symbol(image);
            sym;
            sym = zebra_symbol_next(sym))
            if(!zebra_symbol_get_count(sym)) {
                zebra_symbol_type_t type = zebra_symbol_get_type(sym);
                const char *data = zebra_symbol_get_data(sym);
                g_signal_emit(self, zebra_gtk_signals[DECODED], 0,
                              type, data);

                /* FIXME skip this when unconnected? */
                gchar *text = g_strconcat(zebra_get_symbol_name(type),
                                          zebra_get_addon_name(type),
                                          ":",
                                          data,
                                          NULL);
                g_signal_emit(self, zebra_gtk_signals[DECODED_TEXT], 0, text);
                g_free(text);
            }
    }

    if(zebra->window) {
        rc = zebra_window_draw(zebra->window, image);
        gtk_widget_queue_draw(GTK_WIDGET(self));
    }
    else
        rc = -1;
    gdk_threads_leave();
    return(rc);
}

static void *zebra_gtk_processing_thread (void *arg)
{
    ZebraGtk *self = ZEBRA_GTK(arg);
    if(!self->_private)
        return(NULL);
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);
    g_object_ref(zebra);
    g_assert(zebra->queue);
    g_async_queue_ref(zebra->queue);

    zebra->scanner = zebra_image_scanner_create();
    g_assert(zebra->scanner);

    /* thread side enabled state */
    gboolean video_enabled = FALSE;
    GValue *msg = NULL;

    while(TRUE) {
        if(!msg)
            msg = g_async_queue_pop(zebra->queue);
        g_assert(G_IS_VALUE(msg));

        GType type = G_VALUE_TYPE(msg);
        if(type == G_TYPE_INT) {
            /* video state change */
            int state = g_value_get_int(msg);
            if(state < 0) {
                /* terminate processing thread */
                g_value_unset(msg);
                g_free(msg);
                msg = NULL;
                break;
            }
            g_assert(state >= 0 && state <= 1);
            video_enabled = (state != 0);
        }
        else if(type == G_TYPE_STRING) {
            /* open new video device */
            const char *video_device = g_value_get_string(msg);
            video_enabled = zebra_gtk_video_open(self, video_device);
        }
        else if(type == GDK_TYPE_PIXBUF) {
            /* scan provided image and broadcast results */
            zebra_image_t *image = zebra_image_create();
            GdkPixbuf *pixbuf = GDK_PIXBUF(g_value_dup_object(msg));
            if(zebra_gtk_image_from_pixbuf(image, pixbuf))
                zebra_gtk_process_image(self, image);
            else
                g_object_unref(pixbuf);
            zebra_image_destroy(image);
        }
        else {
            gchar *dbg = g_strdup_value_contents(msg);
            g_warning("unknown message type (%x) passed to thread: %s\n",
                      (unsigned)type, dbg);
            g_free(dbg);
        }
        g_value_unset(msg);
        g_free(msg);
        msg = NULL;

        if(video_enabled) {
            /* release reference to any previous pixbuf */
            zebra_window_draw(zebra->window, NULL);

            if(zebra_video_enable(zebra->video, 1)) {
                zebra_video_error_spew(zebra->video, 0);
                video_enabled = FALSE;
                continue;
            }
            zebra_image_scanner_enable_cache(zebra->scanner, 1);

            while(video_enabled &&
                  !(msg = g_async_queue_try_pop(zebra->queue))) {
                zebra_image_t *image = zebra_video_next_image(zebra->video);
                if(zebra_gtk_process_image(self, image) < 0)
                    video_enabled = FALSE;
                if(image)
                    zebra_image_destroy(image);
            }

            zebra_image_scanner_enable_cache(zebra->scanner, 0);
            if(zebra_video_enable(zebra->video, 0)) {
                zebra_video_error_spew(zebra->video, 0);
                video_enabled = FALSE;
            }
            /* release video image and revert to logo */
            if(zebra->window) {
                zebra_window_draw(zebra->window, NULL);
                gtk_widget_queue_draw(GTK_WIDGET(self));
            }

            if(!video_enabled)
                /* must have been an error while streaming */
                zebra_gtk_video_open(self, NULL);
        }
    }
    if(zebra->window)
        zebra_window_draw(zebra->window, NULL);
    g_object_unref(zebra);
    return(NULL);
}

static void zebra_gtk_realize (GtkWidget *widget)
{
    ZebraGtk *self = ZEBRA_GTK(widget);
    if(!self->_private)
        return;
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    GTK_WIDGET_UNSET_FLAGS(widget, GTK_DOUBLE_BUFFERED);
    GTK_WIDGET_SET_FLAGS(widget, GTK_REALIZED);

    GdkWindowAttr attributes;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.event_mask = (gtk_widget_get_events(widget) |
                             GDK_EXPOSURE_MASK);

    widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
                                    &attributes,
                                    GDK_WA_X | GDK_WA_Y);
    gdk_window_set_user_data(widget->window, widget);
    gdk_window_set_back_pixmap(widget->window, NULL, TRUE);

    /* attach zebra_window to underlying X window */
    if(zebra_window_attach(zebra->window,
                           gdk_x11_drawable_get_xdisplay(widget->window),
                           gdk_x11_drawable_get_xid(widget->window)))
        zebra_window_error_spew(zebra->window, 0);
}

static inline GValue *zebra_gtk_new_value (GType type)
{
    return(g_value_init(g_malloc0(sizeof(GValue)), type));
}

static void zebra_gtk_unrealize (GtkWidget *widget)
{
    if(GTK_WIDGET_MAPPED(widget))
        gtk_widget_unmap(widget);

    ZebraGtk *self = ZEBRA_GTK(widget);
    if(!self->_private)
        return;
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    if(zebra->video_enabled) {
        zebra->video_enabled = FALSE;
        GValue *msg = zebra_gtk_new_value(G_TYPE_INT);
        g_value_set_int(msg, 0);
        g_async_queue_push(zebra->queue, msg);
    }

    zebra_window_attach(zebra->window, NULL, 0);

    GTK_WIDGET_UNSET_FLAGS(widget, GTK_REALIZED);

    gdk_window_set_user_data(widget->window, NULL);
    gdk_window_destroy(widget->window);
    widget->window = NULL;
}

static void zebra_gtk_size_request (GtkWidget *widget,
                                    GtkRequisition *requisition)
{
    ZebraGtk *self = ZEBRA_GTK(widget);
    if(!self->_private)
        return;
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    /* use native video size (max) if available,
     * arbitrary defaults otherwise.
     * video attributes maintained under main gui thread lock
     */
    requisition->width = zebra->req_width;
    requisition->height = zebra->req_height;
}

static void zebra_gtk_size_allocate (GtkWidget *widget,
                                     GtkAllocation *allocation)
{
    ZebraGtk *self = ZEBRA_GTK(widget);
    if(!self->_private)
        return;
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    (*GTK_WIDGET_CLASS(zebra_gtk_parent_class)->size_allocate)
        (widget, allocation);
    if(zebra->window)
        zebra_window_resize(zebra->window,
                            allocation->width, allocation->height);
}

static gboolean zebra_gtk_expose (GtkWidget *widget,
                                  GdkEventExpose *event)
{
    ZebraGtk *self = ZEBRA_GTK(widget);
    if(!self->_private)
        return(FALSE);
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    if(GTK_WIDGET_VISIBLE(widget) &&
       GTK_WIDGET_MAPPED(widget) &&
       zebra_window_redraw(zebra->window))
        return(TRUE);
    return(FALSE);
}

void zebra_gtk_scan_image (ZebraGtk *self,
                           GdkPixbuf *img)
{
    if(!self->_private)
        return;
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    g_object_ref(G_OBJECT(img));

    /* queue for scanning by the processor thread */
    GValue *msg = zebra_gtk_new_value(GDK_TYPE_PIXBUF);

    /* this grabs a new reference to the image,
     * eventually released by the processor thread
     */
    g_value_set_object(msg, img);
    g_async_queue_push(zebra->queue, msg);
}


const char *zebra_gtk_get_video_device (ZebraGtk *self)
{
    if(!self->_private)
        return(NULL);
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);
    if(zebra->video_device)
        return(zebra->video_device);
    else
        return("");
}

void zebra_gtk_set_video_device (ZebraGtk *self,
                                 const char *video_device)
{
    if(!self->_private)
        return;
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    g_free((void*)zebra->video_device);
    zebra->video_device = g_strdup(video_device);
    zebra->video_enabled = video_device && video_device[0];

    /* push another copy to processor thread */
    GValue *msg = zebra_gtk_new_value(G_TYPE_STRING);
    if(video_device)
        g_value_set_string(msg, video_device);
    else
        g_value_set_static_string(msg, "");
    g_async_queue_push(zebra->queue, msg);

    g_object_freeze_notify(G_OBJECT(self));
    g_object_notify(G_OBJECT(self), "video-device");
    g_object_notify(G_OBJECT(self), "video-enabled");
    g_object_thaw_notify(G_OBJECT(self));
}

gboolean zebra_gtk_get_video_enabled (ZebraGtk *self)
{
    if(!self->_private)
        return(FALSE);
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);
    return(zebra->video_enabled);
}

void zebra_gtk_set_video_enabled (ZebraGtk *self,
                                  gboolean video_enabled)
{
    if(!self->_private)
        return;
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    video_enabled = (video_enabled != FALSE);
    if(zebra->video_enabled != video_enabled) {
        zebra->video_enabled = video_enabled;

        /* push state change to processor thread */
        GValue *msg = zebra_gtk_new_value(G_TYPE_INT);
        g_value_set_int(msg, zebra->video_enabled);
        g_async_queue_push(zebra->queue, msg);

        g_object_notify(G_OBJECT(self), "video-enabled");
    }
}

gboolean zebra_gtk_get_video_opened (ZebraGtk *self)
{
    if(!self->_private)
        return(FALSE);
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);
    return(zebra->video_opened);
}

static void zebra_gtk_set_property (GObject *object,
                                    guint prop_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
    ZebraGtk *self = ZEBRA_GTK(object);
    switch(prop_id) {
    case PROP_VIDEO_DEVICE:
        zebra_gtk_set_video_device(self, g_value_get_string(value));
        break;
    case PROP_VIDEO_ENABLED:
        zebra_gtk_set_video_enabled(self, g_value_get_boolean(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void zebra_gtk_get_property (GObject *object,
                                    guint prop_id,
                                    GValue *value,
                                    GParamSpec *pspec)
{
    ZebraGtk *self = ZEBRA_GTK(object);
    if(!self->_private)
        return;
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    switch(prop_id) {
    case PROP_VIDEO_DEVICE:
        if(zebra->video_device)
            g_value_set_string(value, zebra->video_device);
        else
            g_value_set_static_string(value, "");
        break;
    case PROP_VIDEO_ENABLED:
        g_value_set_boolean(value, zebra->video_enabled);
        break;
    case PROP_VIDEO_OPENED:
        g_value_set_boolean(value, zebra->video_opened);
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void zebra_gtk_init (ZebraGtk *self)
{
    ZebraGtkPrivate *zebra = g_object_new(ZEBRA_TYPE_GTK_PRIVATE, NULL);
    self->_private = (void*)zebra;

    zebra->window = zebra_window_create();
    g_assert(zebra->window);

    zebra->req_width = DEFAULT_WIDTH;
    zebra->req_height = DEFAULT_HEIGHT;

    /* spawn a thread to handle decoding and video */
    zebra->queue = g_async_queue_new();
    zebra->thread = g_thread_create(zebra_gtk_processing_thread, self,
                                    FALSE, NULL);
    g_assert(zebra->thread);
}

static void zebra_gtk_dispose (GObject *object)
{
    ZebraGtk *self = ZEBRA_GTK(object);
    if(!self->_private)
        return;

    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);
    self->_private = NULL;

    g_free((void*)zebra->video_device);
    zebra->video_device = NULL;

    /* signal processor thread to exit */
    GValue *msg = zebra_gtk_new_value(G_TYPE_INT);
    g_value_set_int(msg, -1);
    g_async_queue_push(zebra->queue, msg);
    zebra->thread = NULL;

    /* there are no external references which might call other APIs */
    g_async_queue_unref(zebra->queue);

    g_object_unref(G_OBJECT(zebra));
}

static void zebra_gtk_private_finalize (GObject *object)
{
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(object);
    if(zebra->window) {
        zebra_window_destroy(zebra->window);
        zebra->window = NULL;
    }
    if(zebra->scanner) {
        zebra_image_scanner_destroy(zebra->scanner);
        zebra->scanner = NULL;
    }
    if(zebra->video) {
        zebra_video_destroy(zebra->video);
        zebra->video = NULL;
    }
    g_async_queue_unref(zebra->queue);
    zebra->queue = NULL;
}

static void zebra_gtk_class_init (ZebraGtkClass *klass)
{
    zebra_gtk_parent_class = g_type_class_peek_parent(klass);

    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->dispose = zebra_gtk_dispose;
    object_class->set_property = zebra_gtk_set_property;
    object_class->get_property = zebra_gtk_get_property;

    GtkWidgetClass *widget_class = (GtkWidgetClass*)klass;
    widget_class->realize = zebra_gtk_realize;
    widget_class->unrealize = zebra_gtk_unrealize;
    widget_class->size_request = zebra_gtk_size_request;
    widget_class->size_allocate = zebra_gtk_size_allocate;
    widget_class->expose_event = zebra_gtk_expose;
    widget_class->unmap = NULL;

    zebra_gtk_signals[DECODED] =
        g_signal_new("decoded",
                     G_TYPE_FROM_CLASS(object_class),
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(ZebraGtkClass, decoded),
                     NULL, NULL,
                     zebra_marshal_VOID__INT_STRING,
                     G_TYPE_NONE, 2,
                     G_TYPE_INT, G_TYPE_STRING);

    zebra_gtk_signals[DECODED_TEXT] =
        g_signal_new("decoded-text",
                     G_TYPE_FROM_CLASS(object_class),
                     G_SIGNAL_RUN_CLEANUP,
                     G_STRUCT_OFFSET(ZebraGtkClass, decoded_text),
                     NULL, NULL,
                     g_cclosure_marshal_VOID__STRING,
                     G_TYPE_NONE, 1,
                     G_TYPE_STRING);

    GParamSpec *p = g_param_spec_string("video-device",
        "Video device",
        "the platform specific name of the video device",
        NULL,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(object_class, PROP_VIDEO_DEVICE, p);

    p = g_param_spec_boolean("video-enabled",
        "Video enabled",
        "controls streaming from the video device",
        FALSE,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(object_class, PROP_VIDEO_ENABLED, p);

    p = g_param_spec_boolean("video-opened",
        "Video opened",
        "current opened state of the video device",
        FALSE,
        G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
    g_object_class_install_property(object_class, PROP_VIDEO_OPENED, p);
}

GType zebra_gtk_get_type ()
{
    static GType type = 0;
    if(!type) {
        static const GTypeInfo info = {
            sizeof(ZebraGtkClass),
            NULL, /* base_init */
            NULL, /* base_finalize */
            (GClassInitFunc)zebra_gtk_class_init,
            NULL, /* class_finalize */
            NULL, /* class_data */
            sizeof(ZebraGtk),
            0,
            (GInstanceInitFunc)zebra_gtk_init,
        };
        type = g_type_register_static(GTK_TYPE_WIDGET, "ZebraGtk", &info, 0);
    }
    return(type);
}

static void zebra_gtk_private_class_init (ZebraGtkPrivateClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = zebra_gtk_private_finalize;
}

static GType zebra_gtk_private_get_type ()
{
    static GType type = 0;
    if(!type) {
        static const GTypeInfo info = {
            sizeof(ZebraGtkPrivateClass),
            NULL, NULL,
            (GClassInitFunc)zebra_gtk_private_class_init,
            NULL, NULL,
            sizeof(ZebraGtkPrivate),
        };
        type = g_type_register_static(G_TYPE_OBJECT, "ZebraGtkPrivate",
                                      &info, 0);
    }
    return(type);
}

GtkWidget *zebra_gtk_new ()
{
    return(GTK_WIDGET(g_object_new(ZEBRA_TYPE_GTK, NULL)));
}
