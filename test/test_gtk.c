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

#include <gtk/gtk.h>
#include <zbar/zbargtk.h>

static GtkWidget *window = NULL;
static GtkWidget *status_image = NULL;
static GtkTextView *results = NULL;
static gchar *open_file = NULL;

int scan_video(void *add_device,
               void *userdata,
               const char *default_device);

/* decode signal callback
 * puts the decoded result in the textbox
 */
static void decoded (GtkWidget *widget,
                     zbar_symbol_type_t symbol,
                     const char *result,
                     gpointer data)
{
    GtkTextBuffer *resultbuf = gtk_text_view_get_buffer(results);
    GtkTextIter end;
    gtk_text_buffer_get_end_iter(resultbuf, &end);
    gtk_text_buffer_insert(resultbuf, &end, zbar_get_symbol_name(symbol), -1);
    gtk_text_buffer_insert(resultbuf, &end, zbar_get_addon_name(symbol), -1);
    gtk_text_buffer_insert(resultbuf, &end, ":", -1);
    gtk_text_buffer_insert(resultbuf, &end, result, -1);
    gtk_text_buffer_insert(resultbuf, &end, "\n", -1);
    gtk_text_view_scroll_to_iter(results, &end, 0, FALSE, 0, 0);
}


/* update botton state when video state changes
 */
static void video_enabled (GObject *object,
                           GParamSpec *param,
                           gpointer data)
{
    ZBarGtk *zbar = ZBAR_GTK(object);
    gboolean enabled = zbar_gtk_get_video_enabled(zbar);
    gboolean opened = zbar_gtk_get_video_opened(zbar);

    GtkToggleButton *button = GTK_TOGGLE_BUTTON(data);
    gboolean active = gtk_toggle_button_get_active(button);
    if(active != (opened && enabled))
        gtk_toggle_button_set_active(button, enabled);
}

static void video_opened (GObject *object,
                          GParamSpec *param,
                          gpointer data)
{
    ZBarGtk *zbar = ZBAR_GTK(object);
    gboolean opened = zbar_gtk_get_video_opened(zbar);
    gboolean enabled = zbar_gtk_get_video_enabled(zbar);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data), opened && enabled);
    gtk_widget_set_sensitive(GTK_WIDGET(data), opened);
}

/* (re)open the video when a new device is selected
 */
static void video_changed (GtkWidget *widget,
                           gpointer data)
{
    const char *video_device =
        gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
    zbar_gtk_set_video_device(ZBAR_GTK(data),
                               ((video_device && video_device[0] != '<')
                                ? video_device
                                : NULL));
}

static void status_button_toggled (GtkToggleButton *button,
                                   gpointer data)
{
    ZBarGtk *zbar = ZBAR_GTK(data);
    gboolean opened = zbar_gtk_get_video_opened(zbar);
    gboolean enabled = zbar_gtk_get_video_enabled(zbar);
    gboolean active = gtk_toggle_button_get_active(button);
    if(opened && (active != enabled))
        zbar_gtk_set_video_enabled(ZBAR_GTK(data), active);
    gtk_image_set_from_stock(GTK_IMAGE(status_image),
                             (opened && active) ? GTK_STOCK_YES : GTK_STOCK_NO,
                             GTK_ICON_SIZE_BUTTON);
    gtk_button_set_label(GTK_BUTTON(button),
                         (!opened) ? "closed" :
                         (active) ? "enabled" : "disabled");
}

static void open_button_clicked (GtkButton *button,
                                 gpointer data)
{
    GtkWidget *dialog =
        gtk_file_chooser_dialog_new("Open Image File", GTK_WINDOW(window),
                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                                    NULL);
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    if(open_file)
        gtk_file_chooser_set_filename(chooser, open_file);

    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        gchar *file = gtk_file_chooser_get_filename(chooser);
        GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(file, NULL);
        if(pixbuf)
            zbar_gtk_scan_image(ZBAR_GTK(data), pixbuf);
        else
            fprintf(stderr, "ERROR: unable to open image file: %s\n", file);

        if(open_file && file)
            g_free(open_file);
        open_file = file;
    }
    gtk_widget_destroy(dialog);
}

/* build a simple gui w/:
 *   - a combo box to select the desired video device
 *   - the zbar widget to display video
 *   - a non-editable text box to display any results
 */
int main (int argc, char *argv[])
{
    g_thread_init(NULL);
    gdk_threads_init();
    gdk_threads_enter();

    gtk_init(&argc, &argv);
    const char *video_arg = NULL;
    if(argc > 1)
        video_arg = argv[1];

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

    gtk_window_set_title(GTK_WINDOW(window), "test_gtk");
    gtk_container_set_border_width(GTK_CONTAINER(window), 8);

    g_signal_connect(G_OBJECT(window), "destroy",
                     G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *zbar = zbar_gtk_new();

    g_signal_connect(G_OBJECT(zbar), "decoded",
                     G_CALLBACK(decoded), NULL);

    /* video device list combo box */
    GtkWidget *video_list = gtk_combo_box_new_text();

    g_signal_connect(G_OBJECT(video_list), "changed",
                     G_CALLBACK(video_changed), zbar);

    /* enable/disable status button */
    GtkWidget *status_button = gtk_toggle_button_new();
    status_image = gtk_image_new_from_stock(GTK_STOCK_NO,
                                            GTK_ICON_SIZE_BUTTON);
    gtk_button_set_image(GTK_BUTTON(status_button), status_image);
    gtk_button_set_label(GTK_BUTTON(status_button), "closed");
    gtk_widget_set_sensitive(status_button, FALSE);

    /* bind status button state and video state */
    g_signal_connect(G_OBJECT(status_button), "toggled",
                     G_CALLBACK(status_button_toggled), zbar);
    g_signal_connect(G_OBJECT(zbar), "notify::video-enabled",
                     G_CALLBACK(video_enabled), status_button);
    g_signal_connect(G_OBJECT(zbar), "notify::video-opened",
                     G_CALLBACK(video_opened), status_button);

    /* open image file button */
    GtkWidget *open_button = gtk_button_new_from_stock(GTK_STOCK_OPEN);

    g_signal_connect(G_OBJECT(open_button), "clicked",
                     G_CALLBACK(open_button_clicked), zbar);

    gtk_combo_box_append_text(GTK_COMBO_BOX(video_list), "<none>");
    int active = scan_video(gtk_combo_box_append_text, video_list, video_arg);
    if(active >= 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(video_list), active);

    /* hbox for combo box and buttons */
    GtkWidget *hbox = gtk_hbox_new(FALSE, 8);

    gtk_box_pack_start(GTK_BOX(hbox), video_list, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), status_button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), open_button, FALSE, FALSE, 0);

    /* text box for holding results */
    results = GTK_TEXT_VIEW(gtk_text_view_new());
    gtk_widget_set_size_request(GTK_WIDGET(results), 320, 64);
    gtk_text_view_set_editable(results, FALSE);
    gtk_text_view_set_cursor_visible(results, FALSE);
    gtk_text_view_set_left_margin(results, 4);

    /* vbox for hbox, zbar test widget and result text box */
    GtkWidget *vbox = gtk_vbox_new(FALSE, 8);
    gtk_container_add(GTK_CONTAINER(window), vbox);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), zbar, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(results), FALSE, FALSE, 0);

    GdkGeometry hints;
    hints.min_width = 320;
    hints.min_height = 240;
    gtk_window_set_geometry_hints(GTK_WINDOW(window), zbar, &hints,
                                  GDK_HINT_MIN_SIZE);

    gtk_widget_show_all(window);
    gtk_main();
    gdk_threads_leave();
    return(0);
}
