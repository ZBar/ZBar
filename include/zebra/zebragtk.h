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
#ifndef __ZEBRA_GTK_H__
#define __ZEBRA_GTK_H__

/** SECTION:ZebraGtk
 * @short_description: barcode reader GTK+ 2.x widget
 * @include: zebra/zebragtk.h
 *
 * embeds a barcode reader directly into a GTK+ based GUI.  the widget
 * can process barcodes from a video source (using the
 * #ZebraGtk:video-device and #ZebraGtk:video-enabled properties) or
 * from individual GdkPixbufs supplied to zebra_gtk_scan_image()
 *
 * Since: 1.5
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

#include <zebra.h>

G_BEGIN_DECLS

#define ZEBRA_TYPE_GTK (zebra_gtk_get_type())
#define ZEBRA_GTK(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), ZEBRA_TYPE_GTK, ZebraGtk))
#define ZEBRA_GTK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), ZEBRA_TYPE_GTK, ZebraGtkClass))
#define ZEBRA_IS_GTK(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZEBRA_TYPE_GTK))
#define ZEBRA_IS_GTK_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), ZEBRA_TYPE_GTK))
#define ZEBRA_GTK_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), ZEBRA_TYPE_GTK, ZebraGtkClass))

typedef struct _ZebraGtk ZebraGtk;
typedef struct _ZebraGtkClass ZebraGtkClass;

struct _ZebraGtk {
    GtkWidget widget;
    gpointer *_private;

    /* properties */

    /**
     * ZebraGtk:video-device:
     *
     * the currently set video device.
     *
     * setting a new device opens it and automatically sets
     * #ZebraGtk:video-enabled.  set the empty string ("") or NULL to
     * close.
     */

    /**
     * ZebraGtk:video-enabled:
     *
     * video device streaming state.
     *
     * use to pause/resume video scanning.
     */

    /**
     * ZebraGtk:video-opened:
     *
     * video device opened state.
     *
     * (re)setting #ZebraGtk:video-device should eventually cause it
     * to be opened or closed.  any errors while streaming/scanning
     * will also cause the device to be closed
     */
};

struct _ZebraGtkClass {
    GtkWidgetClass parent_class;

    /* signals */

    /**
     * ZebraGtk::decoded:
     * @widget: the object that received the signal
     * @symbol_type: the type of symbol decoded (a zebra_symbol_type_t)
     * @data: the data decoded from the symbol
     * 
     * emitted when a barcode is decoded from an image.
     * the symbol type and contained data are provided as separate
     * parameters
     */
    void (*decoded) (ZebraGtk *zebra,
                     zebra_symbol_type_t symbol_type,
                     const char *data);

    /**
     * ZebraGtk::decoded-text:
     * @widget: the object that received the signal
     * @text: the decoded data prefixed by the string name of the
     *   symbol type (separated by a colon)
     *
     * emitted when a barcode is decoded from an image.
     * the symbol type name is prefixed to the data, separated by a
     * colon
     */
    void (*decoded_text) (ZebraGtk *zebra,
                          const char *text);

    /**
     * ZebraGtk::scan-image:
     * @widget: the object that received the signal
     * @image: the image to scan for barcodes
     */
    void (*scan_image) (ZebraGtk *zebra,
                        GdkPixbuf *image);
};

GType zebra_gtk_get_type() G_GNUC_CONST;

/** 
 * zebra_gtk_new:
 * create a new barcode reader widget instance.
 * initially has no associated video device or image.
 *
 * Returns: a new #ZebraGtk widget instance
 */
GtkWidget *zebra_gtk_new();

/**
 * zebra_gtk_scan_image:
 * 
 */
void zebra_gtk_scan_image(ZebraGtk *zebra,
                          GdkPixbuf *image);

/** retrieve the currently opened video device.
 *
 * Returns: the current video device or NULL if no device is opened
 */
const char *zebra_gtk_get_video_device(ZebraGtk *zebra);

/** open a new video device.
 *
 * @video_device: the platform specific name of the device to open.
 *   use NULL to close a currently opened device.
 *
 * @note since opening a device may take some time, this call will
 * return immediately and the device will be opened asynchronously
 */
void zebra_gtk_set_video_device(ZebraGtk *zebra,
                                const char *video_device);

/** retrieve the current video enabled state.
 *
 * Returns: true if video scanning is currently enabled, false otherwise
 */
gboolean zebra_gtk_get_video_enabled(ZebraGtk *zebra);

/** enable/disable video scanning.
 * @video_enabled: true to enable video scanning, false to disable
 *
 * has no effect unless a video device is opened
 */
void zebra_gtk_set_video_enabled(ZebraGtk *zebra,
                                 gboolean video_enabled);

/** retrieve the current video opened state.
 *
 * Returns: true if video device is currently opened, false otherwise
 */
gboolean zebra_gtk_get_video_opened(ZebraGtk *zebra);

/** 
 * utility function to populate a zebra_image_t from a GdkPixbuf.
 * @image: the zebra library image destination to populate
 * @pixbuf: the GdkPixbuf source
 *
 * Returns: TRUE if successful or FALSE if the conversion could not be
 * performed for some reason
 */
gboolean zebra_gtk_image_from_pixbuf(zebra_image_t *image,
                                     GdkPixbuf *pixbuf);

G_END_DECLS

#endif
