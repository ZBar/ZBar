AM_CPPFLAGS = -I$(srcdir)/include
AM_CFLAGS = -Wall -Wno-parentheses
AM_CXXFLAGS = $(AM_CFLAGS)
ACLOCAL_AMFLAGS = -I config
bin_PROGRAMS =
check_PROGRAMS =
EXTRA_PROGRAMS =
lib_LTLIBRARIES =
pyexec_LTLIBRARIES =
CLEANFILES =
DISTCLEANFILES =
BUILT_SOURCES =
EXTRA_DIST =

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = zbar.pc

dist_doc_DATA = COPYING HACKING INSTALL LICENSE NEWS README TODO

include $(srcdir)/include/Makefile.am.inc
include $(srcdir)/zbar/Makefile.am.inc
if HAVE_MAGICK
include $(srcdir)/zbarimg/Makefile.am.inc
endif
if HAVE_VIDEO
include $(srcdir)/zbarcam/Makefile.am.inc
endif
if HAVE_PYTHON
include $(srcdir)/python/Makefile.am.inc
endif
if HAVE_GTK
include $(srcdir)/gtk/Makefile.am.inc
pkgconfig_DATA += zbar-gtk.pc
if HAVE_PYTHON
include $(srcdir)/pygtk/Makefile.am.inc
endif
endif
if HAVE_QT
include $(srcdir)/qt/Makefile.am.inc
pkgconfig_DATA += zbar-qt.pc
endif
if HAVE_NPAPI
include $(srcdir)/plugin/Makefile.am.inc
endif
include $(srcdir)/test/Makefile.am.inc
include $(srcdir)/doc/Makefile.am.inc

EXTRA_DIST += zbar.ico zbar.nsi

EXTRA_DIST += examples/upcrpc.pl examples/upcrpc.py examples/paginate.pl \
    examples/barcode.png examples/processor.pl examples/processor.py \
    examples/read_one.py examples/read_one.pl \
    examples/scan_image.c examples/scan_image.cpp examples/scan_image.pl \
    examples/scan_image.py examples/scan_image.vcproj

EXTRA_DIST += perl/MANIFEST perl/README perl/Changes perl/COPYING.LIB \
    perl/Makefile.PL perl/typemap perl/ZBar.xs perl/ppport.h \
    perl/ZBar.pm perl/inc/Devel/CheckLib.pm perl/ZBar/Image.pod \
    perl/ZBar/ImageScanner.pod perl/ZBar/Processor.pod perl/ZBar/Symbol.pod \
    perl/examples/paginate.pl perl/examples/processor.pl \
    perl/examples/read_one.pl perl/examples/scan_image.pl \
    perl/t/barcode.png perl/t/ZBar.t perl/t/Decoder.t perl/t/Image.t \
    perl/t/Processor.t perl/t/Scanner.t perl/t/pod.t perl/t/pod-coverage.t

if WIN32
dist_doc_DATA += README.windows
pkgdata_DATA = $(srcdir)/examples/barcode.png \
    $(srcdir)/examples/scan_image.cpp $(srcdir)/examples/scan_image.vcproj

%-rc.o: %.rc
	$(RC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) -o $@ $<
%-rc.lo: %.rc
	$(LIBTOOL) --tag=RC $(AM_LIBTOOLFLAGS) $(LIBTOOLFLAGS) \
	--mode=compile $(RC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) \
	$(AM_CPPFLAGS) $(CPPFLAGS) -o $@ $<

# install to tmp dest and run NSIS to generate installer
dist-nsis: html-local
	test ! -e _nsis || test -d _nsis && rm -rf _nsis
	mkdir _nsis
	tmpinst=`cd _nsis && pwd | sed -e 's,^[^:\\/]:[\\/],/,'` \
	  && $(MAKE) $(AM_MAKEFLAGS) DESTDIR=$$tmpinst prefix=/ install
	cp zbar/.libs/libzbar-0.dll.def _nsis/lib/libzbar-0.def
	cp -r doc/html _nsis/share/doc/zbar/
	$(WINEXEC) lib.exe /machine:x86 /def:_nsis/lib/libzbar-0.def /out:_nsis/lib/libzbar-0.lib
	cd _nsis && \
	  makensis -NOCD -V2 -DVERSION=$(VERSION) $(abs_srcdir)/zbar.nsi
	@ls -l _nsis/zbar-$(VERSION)-setup.exe
endif
