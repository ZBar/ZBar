check_PROGRAMS += test/test_decode
test_test_decode_CFLAGS = -Wno-unused $(AM_CFLAGS)
test_test_decode_LDADD = zebra/libzebra.la $(AM_LDADD)

check_PROGRAMS += test/test_proc
test_test_proc_SOURCES = test/test_proc.c \
    test/test_images.c test/test_images.h
test_test_proc_LDADD = zebra/libzebra.la $(AM_LDADD)

check_PROGRAMS += test/test_convert
test_test_convert_SOURCES = test/test_convert.c \
    test/test_images.c test/test_images.h
test_test_convert_LDADD = zebra/libzebra.la $(AM_LDADD)

check_PROGRAMS += test/test_cpp
test_test_cpp_SOURCES = test/test_cpp.cpp
test_test_cpp_LDADD = zebra/libzebra.la $(AM_LDADD)

TEST_IMAGE_SOURCES = test/test_images.c test/test_images.h

if HAVE_X
check_PROGRAMS += test/test_window
test_test_window_SOURCES = test/test_window.c $(TEST_IMAGE_SOURCES)
test_test_window_LDADD = zebra/libzebra.la $(AM_LDADD)
test_test_window_CPPFLAGS = -I$(srcdir)/zebra $(AM_CPPFLAGS)
endif

if HAVE_VIDEO
check_PROGRAMS += test/test_video
test_test_video_SOURCES = test/test_video.c $(TEST_IMAGE_SOURCES)
test_test_video_LDADD = -lrt zebra/libzebra.la $(AM_LDADD)
test_test_video_CPPFLAGS = -I$(srcdir)/zebra $(AM_CPPFLAGS)
endif

if HAVE_MAGICK
check_PROGRAMS += test/dbg_scan
test_dbg_scan_SOURCES = test/dbg_scan.cpp
test_dbg_scan_CPPFLAGS = $(MAGICK_CFLAGS) $(AM_CPPFLAGS)
test_dbg_scan_LDADD = $(MAGICK_LIBS) zebra/libzebra.la $(AM_LDADD)
endif

if HAVE_GTK
check_PROGRAMS += test/test_gtk
test_test_gtk_SOURCES = test/test_gtk.c
test_test_gtk_CPPFLAGS = $(GTK_CFLAGS) $(AM_CPPFLAGS)
test_test_gtk_LDADD = $(GTK_LIBS) gtk/libzebragtk.la $(AM_LDADD)
endif

EXTRA_DIST += test/test_pygtk.py

# automake bug in "monolithic mode"?
CLEANFILES += test/.libs/test_decode test/.libs/test_proc \
    test/.libs/test_convert test/.libs/test_window \
    test/.libs/test_video test/.libs/dbg_scan test/.libs/test_gtk
