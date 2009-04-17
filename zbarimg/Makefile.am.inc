bin_PROGRAMS += zbarimg/zbarimg
zbarimg_zbarimg_SOURCES = zbarimg/zbarimg.cpp
zbarimg_zbarimg_CPPFLAGS = $(MAGICK_CFLAGS) $(AM_CPPFLAGS)
zbarimg_zbarimg_LDADD = $(MAGICK_LIBS) zbar/libzbar.la
# automake bug in "monolithic mode"?
CLEANFILES += zbarimg/.libs/zbarimg
