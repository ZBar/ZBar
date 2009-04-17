check_PROGRAMS += test/test_decode
test_test_decode_SOURCES = test/test_decode.c test/pdf417_encode.h
test_test_decode_CFLAGS = -Wno-unused $(AM_CFLAGS)
test_test_decode_LDADD = zbar/libzbar.la $(AM_LDADD)

TEST_IMAGE_SOURCES = test/test_images.c test/test_images.h

check_PROGRAMS += test/test_convert
test_test_convert_SOURCES = test/test_convert.c $(TEST_IMAGE_SOURCES)
test_test_convert_LDADD = zbar/libzbar.la $(AM_LDADD)

if HAVE_POLL
check_PROGRAMS += test/test_proc
test_test_proc_SOURCES = test/test_proc.c $(TEST_IMAGE_SOURCES)
test_test_proc_LDADD = zbar/libzbar.la $(AM_LDADD)

check_PROGRAMS += test/test_cpp
test_test_cpp_SOURCES = test/test_cpp.cpp
test_test_cpp_LDADD = zbar/libzbar.la $(AM_LDADD)

if HAVE_JPEG
check_PROGRAMS += test/test_jpeg
test_test_jpeg_SOURCES = test/test_jpeg.c
test_test_jpeg_LDADD = zbar/libzbar.la $(AM_LDADD)
endif
endif

if HAVE_MAGICK
check_PROGRAMS += test/dbg_scan
test_dbg_scan_SOURCES = test/dbg_scan.cpp
test_dbg_scan_CPPFLAGS = $(MAGICK_CFLAGS) $(AM_CPPFLAGS)
test_dbg_scan_LDADD = $(MAGICK_LIBS) zbar/libzbar.la $(AM_LDADD)
endif

if HAVE_GTK
check_PROGRAMS += test/test_gtk
test_test_gtk_SOURCES = test/test_gtk.c test/scan_video.c
test_test_gtk_CPPFLAGS = $(GTK_CFLAGS) $(AM_CPPFLAGS)
test_test_gtk_LDADD = $(GTK_LIBS) gtk/libzbargtk.la $(AM_LDADD)
endif

if HAVE_QT
check_PROGRAMS += test/test_qt
test_test_qt_SOURCES = test/test_qt.cpp test/scan_video.c
nodist_test_test_qt_SOURCES = test/moc_test_qt.h
test_test_qt_CPPFLAGS = -Itest $(QT_CFLAGS) $(AM_CPPFLAGS)
test_test_qt_LDADD = $(QT_LIBS) qt/libzbarqt.la $(AM_LDADD)
BUILT_SOURCES += $(nodist_test_test_qt_SOURCES)
DISTCLEANFILES += $(nodist_test_test_qt_SOURCES)

test/moc_%.h: test/%.cpp
	$(MOC) -i $(test_test_qt_CPPFLAGS) $< -o $@
endif

EXTRA_DIST += test/test_pygtk.py test/test_perl.pl

# automake bug in "monolithic mode"?
CLEANFILES += test/.libs/test_decode test/.libs/test_proc \
    test/.libs/test_convert test/.libs/test_window \
    test/.libs/test_video test/.libs/dbg_scan test/.libs/test_gtk
