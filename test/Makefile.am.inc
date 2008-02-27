check_PROGRAMS += test/test_decode
test_test_decode_CFLAGS = -Wno-unused
test_test_decode_LDADD = zebra/libzebra.la

check_PROGRAMS += test/test_proc
test_test_proc_SOURCES = test/test_proc.c \
    test/test_images.c test/test_images.h
test_test_proc_LDADD = zebra/libzebra.la

check_PROGRAMS += test/test_convert
test_test_convert_SOURCES = test/test_convert.c \
    test/test_images.c test/test_images.h
test_test_convert_LDADD = zebra/libzebra.la

if HAVE_X
check_PROGRAMS += test/test_window
test_test_window_SOURCES = test/test_window.c \
    test/test_images.c test/test_images.h
test_test_window_LDADD = zebra/libzebra.la
test_test_window_CPPFLAGS = -I$(srcdir)/zebra $(AM_CPPFLAGS)
endif

if HAVE_VIDEO
check_PROGRAMS += test/test_video
test_test_video_SOURCES = test/test_video.c \
    test/test_images.c test/test_images.h
test_test_video_LDADD = -lrt zebra/libzebra.la
test_test_video_CPPFLAGS = -I$(srcdir)/zebra $(AM_CPPFLAGS)
endif

if HAVE_MAGICK
check_PROGRAMS += test/dbg_scan
test_dbg_scan_SOURCES = test/dbg_scan.cc
test_dbg_scan_CPPFLAGS = $(MAGICK_CPPFLAGS) $(AM_CPPFLAGS)
test_dbg_scan_CXXFLAGS = $(MAGICK_CXXFLAGS) $(AM_CXXFLAGS)
test_dbg_scan_LDFLAGS = $(MAGICK_LDFLAGS) $(AM_LDFLAGS)
test_dbg_scan_LDADD = $(MAGICK_LIBS) zebra/libzebra.la
endif

# automake bug in "monolithic mode"?
CLEANFILES += test/.libs/test_decode test/.libs/test_proc \
    test/.libs/test_convert test/.libs/test_window \
    test/.libs/test_video test/.libs/dbg_scan
