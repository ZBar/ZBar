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

#include <stdio.h>
#include <assert.h>

#include <gtk/gtksignal.h>
#include <gdk/gdkx.h>

#include <zebra/zebragtk.h>
#include "zebragtkprivate.h"
#include "zebramarshal.h"

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

GQuark zebra_gtk_error_quark ()
{
    return(g_quark_from_static_string("zebra_gtk_error"));
}

enum {
    DECODED,
    LAST_SIGNAL
};

enum {
    PROP_0,
    PROP_VIDEO_DEVICE,
    PROP_VIDEO_ENABLED,
};

enum {
    IDLE,               /* no images to process */
    VIDEO_INIT,         /* new video_device available to open */
    VIDEO_READY,        /* video open, ready to capture/process images */
    IMAGE,              /* single image available to process */
    EXIT = -1           /* finish thread */
};

static guint zebra_gtk_signals[LAST_SIGNAL] = { 0 };

static gpointer zebra_gtk_parent_class = NULL;

/* FIXME what todo w/errors? OOM? */


static inline int zebra_gtk_video_open (ZebraGtk *self,
                                        const char *video_device)
{
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    /* (re)create video
     * FIXME video should support re-open
     */
    if(zebra->video) {
        /* ensure old video doesn't have image ref
         * (FIXME handle video destroyed w/images outstanding)
         */
        gdk_threads_enter();
        if(zebra->window)
            zebra_window_draw(zebra->window, NULL);
        gdk_threads_leave();
        
        zebra_video_destroy(zebra->video);
        zebra->video = NULL;
    }
    zebra->video = zebra_video_create();
    assert(zebra->video);

    int rc = zebra_video_open(zebra->video, video_device);
    if(!rc) {
        /* negotiation accesses the window format list */
        gdk_threads_enter();
        zebra_negotiate_format(zebra->video, zebra->window);
        gtk_widget_queue_resize(GTK_WIDGET(self));
        gdk_threads_leave();
    }
    else
        zebra_video_error_spew(zebra->video, 0);

    return(rc);
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

    if(rc) {
        /* update decode results */
        const zebra_symbol_t *sym;
        for(sym = zebra_image_first_symbol(image);
            sym;
            sym = zebra_symbol_next(sym))
            if(!zebra_symbol_get_count(sym)) {
                char *data = g_strdup(zebra_symbol_get_data(sym));
                g_signal_emit(self, zebra_gtk_signals[DECODED], 0,
                              zebra_symbol_get_type(sym), data);
                g_free(data);
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

    zebra->scanner = zebra_image_scanner_create();
    assert(zebra->scanner);

    g_mutex_lock(zebra->mutex);
    while(zebra->state != EXIT) {
        while(zebra->state == IDLE ||
              (zebra->state == VIDEO_READY && !zebra->video_enabled))
            g_cond_wait(zebra->cond, zebra->mutex);

        if(zebra->state == VIDEO_INIT) {
            const char *video_device = (const char *)zebra->video_device;
            const char *video_device_save = g_strdup(video_device);
            g_mutex_unlock(zebra->mutex);

            int rc = zebra_gtk_video_open(self, video_device_save);

            g_mutex_lock(zebra->mutex);
            if(zebra->state == VIDEO_INIT &&
               video_device == zebra->video_device)
                zebra->state = (!rc) ? VIDEO_READY : IDLE /* ERROR? */;
            g_free((void*)video_device_save);
        }

        gboolean video_enabled = FALSE;
        if(zebra->state == VIDEO_READY && zebra->video_enabled) {
            g_mutex_unlock(zebra->mutex);
            video_enabled = !zebra_video_enable(zebra->video, 1);
            if(video_enabled)
                zebra_image_scanner_enable_cache(zebra->scanner, 1);
            else {
                zebra_video_error_spew(zebra->video, 0);
                zebra->state = IDLE;
            }
            g_mutex_lock(zebra->mutex);
        }

        while(zebra->state == VIDEO_READY && zebra->video_enabled) {
            g_mutex_unlock(zebra->mutex);

            zebra_image_t *image = zebra_video_next_image(zebra->video);
            int rc = zebra_gtk_process_image(self, image);
            if(image)
                zebra_image_destroy(image);

            g_mutex_lock(zebra->mutex);
            if(zebra->state == VIDEO_READY && rc < 0)
                zebra->state = IDLE /* ERROR? */;
        }

        if(video_enabled) {
            g_mutex_unlock(zebra->mutex);
            zebra_image_scanner_enable_cache(zebra->scanner, 0);
            if(zebra_video_enable(zebra->video, 0))
                zebra_video_error_spew(zebra->video, 0);
            g_mutex_lock(zebra->mutex);
        }
    }
    g_mutex_unlock(zebra->mutex);
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

    g_mutex_lock(zebra->mutex);
    if(zebra->video_device)
        zebra->state = VIDEO_INIT;
    g_cond_signal(zebra->cond);
    g_mutex_unlock(zebra->mutex);
}

static void zebra_gtk_unrealize (GtkWidget *widget)
{
    if(GTK_WIDGET_MAPPED(widget))
        gtk_widget_unmap(widget);

    ZebraGtk *self = ZEBRA_GTK(widget);
    if(!self->_private)
        return;
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    g_mutex_lock(zebra->mutex);
    zebra->state = IDLE;
    g_cond_signal(zebra->cond);
    g_mutex_unlock(zebra->mutex);

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

    g_mutex_lock(zebra->mutex);
    if(zebra->video) {
        /* use native video size (max) if available */
        requisition->width = zebra_video_get_width(zebra->video);
        requisition->height = zebra_video_get_height(zebra->video);
    }
    g_mutex_unlock(zebra->mutex);

    if(!zebra->video || !requisition->width || !requisition->height) {
        /* use arbitrary defaults if no video available */
        requisition->width = DEFAULT_WIDTH;
        requisition->height = DEFAULT_HEIGHT;
    }
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


void zebra_gtk_set_video_device (ZebraGtk *self,
                                 const char *video_device)
{
    if(!self->_private)
        return;
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);
    g_mutex_lock(zebra->mutex);
    g_free((void*)zebra->video_device);
    zebra->video_device = g_strdup(video_device);
    zebra->video_enabled = TRUE;
    if(GTK_WIDGET_REALIZED(GTK_WIDGET(self))) {
        zebra->state = VIDEO_INIT;
        g_cond_signal(zebra->cond);
    }
    g_mutex_unlock(zebra->mutex);
    g_object_notify(G_OBJECT(self), "video-device");
}

void zebra_gtk_set_video_enabled (ZebraGtk *self,
                                  gboolean video_enabled)
{
    if(!self->_private)
        return;
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);
    video_enabled = (video_enabled != FALSE);

    g_mutex_lock(zebra->mutex);
    if(zebra->video_enabled != video_enabled) {
        zebra->video_enabled = video_enabled;
        g_cond_signal(zebra->cond);
        g_mutex_unlock(zebra->mutex);
        g_object_notify(G_OBJECT(self), "video-enabled");
    }
    else
        g_mutex_unlock(zebra->mutex);
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
        g_mutex_lock(zebra->mutex);
        g_value_set_string(value, (const char*)zebra->video_device);
        g_mutex_unlock(zebra->mutex);
        break;
    case PROP_VIDEO_ENABLED:
        g_mutex_lock(zebra->mutex);
        g_value_set_boolean(value, zebra->video_enabled);
        g_mutex_unlock(zebra->mutex);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    }
}

static void zebra_gtk_init (ZebraGtk *self)
{
    ZebraGtkPrivate *zebra = g_object_new(ZEBRA_TYPE_GTK_PRIVATE, NULL);
    self->_private = (void*)zebra;

    zebra->window = zebra_window_create();
    assert(zebra->window);

    /* spawn a thread to handle decoding and video */
    zebra->mutex = g_mutex_new();
    assert(zebra->mutex);
    zebra->cond = g_cond_new();
    assert(zebra->cond);
    zebra->thread = g_thread_create(zebra_gtk_processing_thread, self,
                                    FALSE, NULL);
    assert(zebra->thread);
}

static void zebra_gtk_dispose (GObject *object)
{
    ZebraGtk *self = ZEBRA_GTK(object);
    if(!self->_private)
        return;

    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(self->_private);

    g_mutex_lock(zebra->mutex);
    zebra->state = EXIT;
    g_cond_signal(zebra->cond);
    g_mutex_unlock(zebra->mutex);

    self->_private = NULL;
    g_object_unref(G_OBJECT(zebra));
}

static void zebra_gtk_private_finalize (GObject *object)
{
    ZebraGtkPrivate *zebra = ZEBRA_GTK_PRIVATE(object);
    if(zebra->window) {
        zebra_window_destroy(zebra->window);
        zebra->window = NULL;
    }
    if(zebra->video) {
        zebra_video_destroy(zebra->video);
        zebra->video = NULL;
    }
    if(zebra->scanner) {
        zebra_image_scanner_destroy(zebra->scanner);
        zebra->scanner = NULL;
    }
    g_free((void*)zebra->video_device);
    zebra->video_device = NULL;
    g_mutex_free(zebra->mutex);
    zebra->mutex = NULL;
    g_cond_free(zebra->cond);
    zebra->cond = NULL;
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
