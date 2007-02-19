bin_PROGRAMS = zebraimg
zebraimg_SOURCES = zebraimg.cpp
AM_CPPFLAGS = -I$(srcdir)/../include $(MAGICK_CPPFLAGS)
AM_CXXFLAGS = -Wall -Werror $(MAGICK_CXXFLAGS)
AM_LDFLAGS = $(MAGICK_LDFLAGS)
LDADD = $(MAGICK_LIBS) ../zebra/libzebra.la
