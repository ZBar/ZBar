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
#include <string.h>
#include <ftw.h>

#include <gtk/gtk.h>

#include <zebra/zebragtk.h>

GtkWidget *video_list = NULL;
GtkTextView *results = NULL;
const char *video_arg = NULL;
int video_list_size = 0;

/* decode signal callback
 * puts the decoded result in the textbox
 */
static void decoded (GtkWidget *widget,
                     zebra_symbol_type_t symbol,
                     const char *result,
                     gpointer data)
{
    if(!results)
        return;
    GtkTextBuffer *resultbuf = gtk_text_view_get_buffer(results);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(resultbuf, &end);
    gtk_text_buffer_insert(resultbuf, &end, zebra_get_symbol_name(symbol), -1);
    gtk_text_buffer_insert(resultbuf, &end, zebra_get_addon_name(symbol), -1);
    gtk_text_buffer_insert(resultbuf, &end, ":", -1);
    gtk_text_buffer_insert(resultbuf, &end, result, -1);
    gtk_text_buffer_insert(resultbuf, &end, "\n", -1);
    gtk_text_view_scroll_to_iter(results, &end, 0, FALSE, 0, 0);
}

/* (re)open the video when a new device is selected
 */
static void video_changed (GtkWidget *widget,
                           gpointer data)
{
    const char *video_device =
        gtk_combo_box_get_active_text(GTK_COMBO_BOX(video_list));

    zebra_gtk_set_video_device((ZebraGtk*)data, video_device);
}

/* search for v4l devices under /dev
 */
static int video_filter (const char *fpath,
                         const struct stat *sb,
                         int typeflag)
{
    if(S_ISCHR(sb->st_mode) && (sb->st_rdev >> 8) == 81 && fpath) {
        int active = video_arg && !strcmp(video_arg, fpath);
        if(strncmp(fpath, "/dev/", 5)) {
            char abs[strlen(fpath) + 6];
            strcpy(abs, "/dev/");
            if(fpath[0] == '/')
                abs[4] = '\0';
            strcat(abs, fpath);
            gtk_combo_box_append_text(GTK_COMBO_BOX(video_list), abs);
            active |= video_arg && !strcmp(video_arg, abs);
        }
        else
            gtk_combo_box_append_text(GTK_COMBO_BOX(video_list), fpath);

        if(active || (!video_list_size && !video_arg)) {
            gtk_combo_box_set_active(GTK_COMBO_BOX(video_list),
                                     video_list_size);
            video_arg = NULL;
        }
        video_list_size++;
    }
    return(0);
}

/* build a simple gui w/:
 *   - a combo box to select the desired video device
 *   - the zebra widget to display video
 *   - a non-editable text box to display any results
 */
int main (int argc, char *argv[])
{
    g_thread_init(NULL);
    gdk_threads_init();
    gdk_threads_enter();

    gtk_init(&argc, &argv);
    if(argc > 1)
        video_arg = argv[1];

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title(GTK_WINDOW(window), "test_gtk");
    gtk_container_set_border_width(GTK_CONTAINER(window), 8);

    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *zebra = zebra_gtk_new();

    g_signal_connect(G_OBJECT(zebra), "decoded",
                     G_CALLBACK(decoded), NULL);

    video_list = gtk_combo_box_new_text();

    g_signal_connect(G_OBJECT(video_list), "changed",
                     G_CALLBACK(video_changed), zebra);

    if(ftw("/dev", video_filter, 4))
        perror("search for video devices failed");
    if(video_arg) {
        gtk_combo_box_append_text(GTK_COMBO_BOX(video_list), video_arg);
        gtk_combo_box_set_active(GTK_COMBO_BOX(video_list), video_list_size);
        video_list_size++;
    }

    results = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_widget_set_size_request(GTK_WIDGET(results), 320, 64);
    gtk_text_view_set_editable(results, FALSE);
    gtk_text_view_set_cursor_visible(results, FALSE);
    gtk_text_view_set_left_margin(results, 4);

    GtkWidget *vbox = gtk_vbox_new(FALSE, 8);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    gtk_box_pack_start(GTK_BOX(vbox), video_list, FALSE, FALSE, 8);
    gtk_widget_show(video_list);

    gtk_box_pack_start(GTK_BOX(vbox), zebra, TRUE, TRUE, 8);
    gtk_widget_show(zebra);

    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(results), FALSE, FALSE, 8);
    gtk_widget_show(GTK_WIDGET(results));

    GdkGeometry hints;
    hints.min_width = 320;
    hints.min_height = 240;
    gtk_window_set_geometry_hints(GTK_WINDOW(window), zebra, &hints,
                                  GDK_HINT_MIN_SIZE);

    gtk_widget_show(vbox);
    gtk_widget_show(window);

    gtk_main();

    results = NULL;
    gdk_threads_leave();
    return(0);
}
