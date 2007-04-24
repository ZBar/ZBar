check_PROGRAMS += test/test_decode
test_test_decode_LDADD = zebra/libzebra.la

check_PROGRAMS += test/test_walk
test_test_walk_LDADD = zebra/libzebra.la

# automake bug in "monolithic mode"?
CLEANFILES += test/.libs/test_decode test/.libs/test_walk

if HAVE_MAGICK

check_PROGRAMS += test/dbg_scan
test_dbg_scan_SOURCES = test/dbg_scan.cc
test_dbg_scan_CPPFLAGS = $(MAGICK_CPPFLAGS) $(AM_CPPFLAGS)
test_dbg_scan_CXXFLAGS = $(MAGICK_CXXFLAGS) $(AM_CXXFLAGS)
test_dbg_scan_LDFLAGS = $(MAGICK_LDFLAGS) $(AM_LDFLAGS)
test_dbg_scan_LDADD = $(MAGICK_LIBS) zebra/libzebra.la
# automake bug in "monolithic mode"?
CLEANFILES += test/.libs/dbg_scan

check_PROGRAMS += test/test_video

endif
