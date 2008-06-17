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

    zebra_video_t *video;
    zebra_window_t *window;
    zebra_image_scanner_t *scanner;

    GThread *thread;
    GMutex *mutex;
    GCond *cond;

    volatile const char *video_device;
    volatile gboolean video_enabled;
    volatile int state;

} ZebraGtkPrivate;


typedef struct _ZebraGtkPrivateClass
{
    GObjectClass parent_class;

} ZebraGtkPrivateClass;


static GType zebra_gtk_private_get_type() G_GNUC_CONST;

G_END_DECLS

#endif
