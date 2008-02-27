AM_CPPFLAGS = -I$(srcdir)/include
AM_CFLAGS = -Wall -Werror
AM_CXXFLAGS = $(AM_CFLAGS)
bin_PROGRAMS =
check_PROGRAMS =
lib_LTLIBRARIES =
CLEANFILES =

dist_doc_DATA = COPYING.LIB HACKING INSTALL NEWS README TODO

include $(srcdir)/include/Makefile.am.inc
include $(srcdir)/zebra/Makefile.am.inc
if HAVE_MAGICK
include $(srcdir)/zebraimg/Makefile.am.inc
endif
if HAVE_VIDEO
include $(srcdir)/zebracam/Makefile.am.inc
endif
if HAVE_MOZILLA
include $(srcdir)/plugin/Makefile.am.inc
endif
include $(srcdir)/test/Makefile.am.inc
include $(srcdir)/doc/Makefile.am.inc

EXTRA_DIST += zebra.spec.in zebra.spec \
    debian/changelog debian/compat debian/control debian/copyright \
    debian/docs debian/libzebra0.install debian/libzebra-dev.install \
    debian/rules debian/shlibs.local debian/zebra-tools.install
