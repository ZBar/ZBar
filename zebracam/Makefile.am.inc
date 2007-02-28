bin_PROGRAMS += zebracam/zebracam
zebracam_zebracam_SOURCES = zebracam/zebracam.c
zebracam_zebracam_CPPFLAGS = $(SDL_CPPFLAGS) $(AM_CPPFLAGS)
zebracam_zebracam_LDADD = $(SDL_LIBS) zebra/libzebra.la
# automake bug in "monolithic mode"?
CLEANFILES += zebracam/.libs/zebracam
