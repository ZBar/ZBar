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


#define ZEBRA_GTK_ERROR zebra_gtk_error_quark()

typedef enum
{
    ZEBRA_GTK_ERROR_FAILED /* FIXME lame generic fallback (currently unused) */
} ZebraGtkError;


/* zebra widget instance */
typedef struct _ZebraGtk ZebraGtk;
typedef struct _ZebraGtkClass ZebraGtkClass;

struct _ZebraGtk {
    GtkWidget widget;

    gpointer *_private;
};

struct _ZebraGtkClass {
    GtkWidgetClass parent_class;

    void (*decoded) (ZebraGtk *zebra,
                     zebra_symbol_type_t symbol_type,
                     const char *data);
};


extern GQuark zebra_gtk_error_quark();
GType zebra_gtk_get_type() G_GNUC_CONST;
GtkWidget *zebra_gtk_new();
void zebra_gtk_set_video_device(ZebraGtk *zebra,
                                const char *video_device);
void zebra_gtk_set_video_enabled(ZebraGtk *zebra,
                                 gboolean video_enabled);

G_END_DECLS

#endif
