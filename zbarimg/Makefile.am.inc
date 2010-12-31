bin_PROGRAMS += zbarimg/zbarimg
zbarimg_zbarimg_SOURCES = zbarimg/zbarimg.c
zbarimg_zbarimg_CPPFLAGS = $(MAGICK_CFLAGS) $(AM_CPPFLAGS)
zbarimg_zbarimg_LDADD = $(MAGICK_LIBS) zbar/libzbar.la
# automake bug in "monolithic mode"?
CLEANFILES += zbarimg/.libs/zbarimg

if WIN32
zbarimg_zbarimg_SOURCES += zbarimg/zbarimg.rc
zbarimg_zbarimg_LDADD += zbarimg/zbarimg-rc.o
endif

EXTRA_DIST += test/barcodetest.py

check-images: zbarimg/zbarimg
	$(PYTHON) $(srcdir)/test/barcodetest.py

regress-images: zbarimg/zbarimg
	suite='$(ZBAR_TESTS)'; \
	if test "x$$suite" = "x"; then suite='http://zbar.sf.net/test'; fi ; \
	$(PYTHON) $(srcdir)/test/barcodetest.py $$suite
