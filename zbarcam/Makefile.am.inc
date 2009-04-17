bin_PROGRAMS += zbarcam/zbarcam
zbarcam_zbarcam_SOURCES = zbarcam/zbarcam.c
zbarcam_zbarcam_LDADD = zbar/libzbar.la
# automake bug in "monolithic mode"?
CLEANFILES += zbarcam/.libs/zbarcam
