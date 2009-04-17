#!/usr/bin/env python
#------------------------------------------------------------------------
#  Copyright 2008-2009 (c) Jeff Brown <spadix@users.sourceforge.net>
#
#  This file is part of the ZBar Bar Code Reader.
#
#  The ZBar Bar Code Reader is free software; you can redistribute it
#  and/or modify it under the terms of the GNU Lesser Public License as
#  published by the Free Software Foundation; either version 2.1 of
#  the License, or (at your option) any later version.
#
#  The ZBar Bar Code Reader is distributed in the hope that it will be
#  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser Public License for more details.
#
#  You should have received a copy of the GNU Lesser Public License
#  along with the ZBar Bar Code Reader; if not, write to the Free
#  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
#  Boston, MA  02110-1301  USA
#
#  http://sourceforge.net/projects/zbar
#------------------------------------------------------------------------
import sys, os, stat
import pygtk, gtk
import zbarpygtk

def decoded(zbar, data):
    """callback invoked when a barcode is decoded by the zbar widget.
    displays the decoded data in the text box
    """
    buf = results.props.buffer
    end = buf.get_end_iter()
    buf.insert(end, data + "\n")
    results.scroll_to_iter(end, 0)

def video_enabled(zbar, param):
    """callback invoked when the zbar widget enables or disables
    video streaming.  updates the status button state to reflect the
    current video state
    """
    enabled = zbar.get_video_enabled()
    if status_button.get_active() != enabled:
        status_button.set_active(enabled)

def video_opened(zbar, param):
    """callback invoked when the zbar widget opens or closes a video
    device.  also called when a device is closed due to error.
    updates the status button state to reflect the current video state
    """
    opened = zbar.get_video_opened()
    status_button.set_sensitive(opened)
    set_status_label(opened, zbar.get_video_enabled())

def video_changed(widget):
    """callback invoked when a new video device is selected from the
    drop-down list.  sets the new device for the zbar widget,
    which will eventually cause it to be opened and enabled
    """
    dev = video_list.get_active_text()
    if dev[0] == '<':
        dev = ''
    zbar.set_video_device(dev)

def status_button_toggled(button):
    """callback invoked when the status button changes state
    (interactively or programmatically).  ensures the zbar widget
    video streaming state is consistent and updates the display of the
    button to represent the current state
    """
    opened = zbar.get_video_opened()
    active = status_button.get_active()
    if opened and (active != zbar.get_video_enabled()):
        zbar.set_video_enabled(active)
    set_status_label(opened, active)
    if active:
        status_image.set_from_stock(gtk.STOCK_YES, gtk.ICON_SIZE_BUTTON)
    else:
        status_image.set_from_stock(gtk.STOCK_NO, gtk.ICON_SIZE_BUTTON)

def open_button_clicked(button):
    """callback invoked when the 'Open' button is clicked.  pops up an
    'Open File' dialog which the user may use to select an image file.
    if the image is successfully opened, it is passed to the zbar
    widget which displays it and scans it for barcodes.  results are
    returned using the same hook used to report video results
    """
    dialog = gtk.FileChooserDialog("Open Image File", window,
                                   gtk.FILE_CHOOSER_ACTION_OPEN,
                                   (gtk.STOCK_CANCEL, gtk.RESPONSE_CANCEL,
                                    gtk.STOCK_OPEN, gtk.RESPONSE_ACCEPT))
    global open_file
    if open_file:
        dialog.set_filename(open_file)
    try:
        if dialog.run() == gtk.RESPONSE_ACCEPT:
            open_file = dialog.get_filename()
            pixbuf = gtk.gdk.pixbuf_new_from_file(open_file)
            if pixbuf:
                zbar.scan_image(pixbuf)
    finally:
        dialog.destroy()

def set_status_label(opened, enabled):
    """update status button label to reflect indicated state."""
    if not opened:
        label = "closed"
    elif enabled:
        label = "enabled"
    else:
        label = "disabled"
    status_button.set_label(label)

open_file = None
video_device = None
if len(sys.argv) > 1:
    video_device = sys.argv[1]

# threads *must* be properly initialized to use zbarpygtk
gtk.gdk.threads_init()
gtk.gdk.threads_enter()

window = gtk.Window()
window.set_title("test_pygtk")
window.set_border_width(8)
window.connect("destroy", gtk.main_quit)

zbar = zbarpygtk.Gtk()
zbar.connect("decoded-text", decoded)

# video device list combo box
video_list = gtk.combo_box_new_text()
video_list.connect("changed", video_changed)

# enable/disable status button
status_button = gtk.ToggleButton("closed")
status_image = gtk.image_new_from_stock(gtk.STOCK_NO, gtk.ICON_SIZE_BUTTON)
status_button.set_image(status_image)
status_button.set_sensitive(False)

# bind status button state and video state
status_button.connect("toggled", status_button_toggled)
zbar.connect("notify::video-enabled", video_enabled)
zbar.connect("notify::video-opened", video_opened)

# open image file button
open_button = gtk.Button(stock=gtk.STOCK_OPEN)
open_button.connect("clicked", open_button_clicked)

# populate video devices in combo box
video_list.append_text("<none>")
video_list.set_active(0)
for (root, dirs, files) in os.walk("/dev"):
    for dev in files:
        path = os.path.join(root, dev)
        if not os.access(path, os.F_OK):
            continue
        info = os.stat(path)
        if stat.S_ISCHR(info.st_mode) and os.major(info.st_rdev) == 81:
            video_list.append_text(path)
            if path == video_device:
                video_list.set_active(len(video_list.get_model()) - 1)
                video_device = None

if video_device is not None:
    video_list.append_text(video_device)
    video_list.set_active(len(video_list.get_model()) - 1)
    video_device = None

# combine combo box and buttons horizontally
hbox = gtk.HBox(spacing=8)
hbox.pack_start(video_list)
hbox.pack_start(status_button, expand=False)
hbox.pack_start(open_button, expand=False)

# text box for holding results
results = gtk.TextView()
results.set_size_request(320, 64)
results.props.editable = results.props.cursor_visible = False
results.set_left_margin(4)

# combine inputs, scanner, and results vertically
vbox = gtk.VBox(spacing=8)
vbox.pack_start(hbox, expand=False)
vbox.pack_start(zbar)
vbox.pack_start(results, expand=False)

window.add(vbox)
window.set_geometry_hints(zbar, min_width=320, min_height=240)
window.show_all()

gtk.main()
gtk.gdk.threads_leave()
