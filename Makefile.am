AM_CPPFLAGS = -I$(srcdir)/include
AM_CFLAGS = -Wall -Werror
bin_PROGRAMS =
check_PROGRAMS =
CLEANFILES =

include $(srcdir)/include/Makefile.am.inc
include $(srcdir)/zebra/Makefile.am.inc
if HAVE_MAGICK
include $(srcdir)/zebraimg/Makefile.am.inc
endif
if HAVE_SDL
include $(srcdir)/zebracam/Makefile.am.inc
endif
include $(srcdir)/test/Makefile.am.inc
#include $(srcdir)/doc/Makefile.am.inc
