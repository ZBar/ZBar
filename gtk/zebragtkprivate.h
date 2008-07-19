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
#ifndef __ZEBRA_GTK_PRIVATE_H__
#define __ZEBRA_GTK_PRIVATE_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtktable.h>

#include <zebra.h>

G_BEGIN_DECLS

#define ZEBRA_TYPE_GTK_PRIVATE (zebra_gtk_private_get_type())
#define ZEBRA_GTK_PRIVATE(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj), ZEBRA_TYPE_GTK_PRIVATE, ZebraGtkPrivate))
#define ZEBRA_GTK_PRIVATE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass), ZEBRA_TYPE_GTK_PRIVATE, ZebraGtkPrivateClass))
#define ZEBRA_IS_GTK_PRIVATE(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj), ZEBRA_TYPE_GTK_PRIVATE))
#define ZEBRA_IS_GTK_PRIVATE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE((klass), ZEBRA_TYPE_GTK_PRIVATE))
#define ZEBRA_GTK_PRIVATE_GET_CLASS(obj) \
    (G_TYPE_INSTANCE_GET_CLASS((obj), ZEBRA_TYPE_GTK_PRIVATE, ZebraGtkPrivateClass))


/* zebra widget processor thread shared/private data */
typedef struct _ZebraGtkPrivate
{
    GObject object;

    /* these are all owned by the main gui thread */
    GThread *thread;
    const char *video_device;
    gboolean video_enabled;

    /* messages are queued from the gui thread to the processor thread.
     * each message is a GValue containing one of:
     * - G_TYPE_INT: state change
     *     1 = video enable
     *     0 = video disable
     *    -1 = terminate processor thread
     * - G_TYPE_STRING: a named video device to open ("" to close)
     * - GDK_TYPE_PIXBUF: an image to scan
     */
    GAsyncQueue *queue;

    /* current processor state is shared:
     * written by processor thread just after opening video or
     * scanning an image, read by main gui thread during size_request.
     * protected by main gui lock
     */
    unsigned req_width, req_height;
    gboolean video_opened;

    /* window is shared: owned by main gui thread.
     * processor thread only calls draw() and negotiate_format().
     * protected by main gui lock (and internal lock)
     */
    zebra_window_t *window;

    /* video and scanner are owned by the processor thread */
    zebra_video_t *video;
    zebra_image_scanner_t *scanner;

} ZebraGtkPrivate;


typedef struct _ZebraGtkPrivateClass
{
    GObjectClass parent_class;

} ZebraGtkPrivateClass;


static GType zebra_gtk_private_get_type() G_GNUC_CONST;

G_END_DECLS

#endif
