AM_CPPFLAGS = -I$(srcdir)/include
AM_CFLAGS = -Wall
AM_CXXFLAGS = $(AM_CFLAGS)
ACLOCAL_AMFLAGS = -I config
bin_PROGRAMS =
check_PROGRAMS =
lib_LTLIBRARIES =
CLEANFILES =
DISTCLEANFILES =
BUILT_SOURCES =
EXTRA_DIST =

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = zebra.pc

dist_doc_DATA = COPYING.LIB HACKING INSTALL NEWS README TODO

include $(srcdir)/include/Makefile.am.inc
include $(srcdir)/zebra/Makefile.am.inc
if HAVE_MAGICK
include $(srcdir)/zebraimg/Makefile.am.inc
endif
if HAVE_VIDEO
include $(srcdir)/zebracam/Makefile.am.inc
endif
if HAVE_GTK
include $(srcdir)/gtk/Makefile.am.inc
pkgconfig_DATA += zebra-gtk.pc
if HAVE_PYTHON
include $(srcdir)/pygtk/Makefile.am.inc
endif
endif
if HAVE_QT
include $(srcdir)/qt/Makefile.am.inc
pkgconfig_DATA += zebra-qt.pc
endif
if HAVE_NPAPI
include $(srcdir)/plugin/Makefile.am.inc
endif
include $(srcdir)/test/Makefile.am.inc
include $(srcdir)/doc/Makefile.am.inc

EXTRA_DIST += examples/upcrpc.pl examples/upcrpc.py \
    zebra.spec.in zebra.spec
