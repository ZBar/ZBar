bin_PROGRAMS += zebraimg/zebraimg
zebraimg_zebraimg_SOURCES = zebraimg/zebraimg.cpp
zebraimg_zebraimg_CPPFLAGS = $(MAGICK_CPPFLAGS) $(AM_CPPFLAGS)
zebraimg_zebraimg_CXXFLAGS = $(MAGICK_CXXFLAGS) $(AM_CXXFLAGS)
zebraimg_zebraimg_LDFLAGS = $(MAGICK_LDFLAGS) $(AM_LDFLAGS)
zebraimg_zebraimg_LDADD = $(MAGICK_LIBS) zebra/libzebra.la
# automake bug in "monolithic mode"?
CLEANFILES += zebraimg/.libs/zebraimg
