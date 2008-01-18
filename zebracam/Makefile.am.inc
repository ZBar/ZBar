bin_PROGRAMS += zebracam/zebracam
zebracam_zebracam_SOURCES = zebracam/zebracam.c
zebracam_zebracam_LDADD = zebra/libzebra.la
# automake bug in "monolithic mode"?
CLEANFILES += zebracam/.libs/zebracam
