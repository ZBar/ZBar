#!/usr/bin/env python
#------------------------------------------------------------------------
#  Copyright 2008 (c) Jeff Brown <spadix@users.sourceforge.net>
#
#  This file is part of the Zebra Barcode Library.
#
#  The Zebra Barcode Library is free software; you can redistribute it
#  and/or modify it under the terms of the GNU Lesser Public License as
#  published by the Free Software Foundation; either version 2.1 of
#  the License, or (at your option) any later version.
#
#  The Zebra Barcode Library is distributed in the hope that it will be
#  useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser Public License for more details.
#
#  You should have received a copy of the GNU Lesser Public License
#  along with the Zebra Barcode Library; if not, write to the Free
#  Software Foundation, Inc., 51 Franklin St, Fifth Floor,
#  Boston, MA  02110-1301  USA
#
#  http://sourceforge.net/projects/zebra
#------------------------------------------------------------------------
import sys
import pygtk
import gtk
import zebrapygtk

def decoded(widget, symbol, data):
    if not results:
        return
    buf = results.props.buffer
    end = buf.get_end_iter()
    buf.insert(end, str(symbol) + ":" + data + "\n")
    results.scroll_to_iter(end, 0)

video_device = "/dev/video0"
if len(sys.argv) > 1:
    video_device = sys.argv[1]

# threads *must* be properly initialized to use zebrapygtk
gtk.gdk.threads_init()
gtk.gdk.threads_enter()

window = gtk.Window()
window.set_title("test_pygtk")
window.connect("delete-event", gtk.main_quit)

zebra = zebrapygtk.Gtk()
zebra.set_video_device(video_device)
zebra.connect("decoded", decoded)

results = gtk.TextView()
results.set_size_request(320, 64)
results.props.editable = results.props.cursor_visible = False
results.set_left_margin(4)

vbox = gtk.VBox()
vbox.pack_start(zebra)
vbox.pack_start(results, expand=False)

window.add(vbox)
window.set_geometry_hints(zebra, min_width=320, min_height=240)
window.show_all()

gtk.main()

# this is important to prevent cleanup issues
# since the zebra::decoded signal is called from a different thread
results = None

gtk.gdk.threads_leave()
