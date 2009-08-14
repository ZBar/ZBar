bin_PROGRAMS += zbarcam/zbarcam
zbarcam_zbarcam_SOURCES = zbarcam/zbarcam.c
zbarcam_zbarcam_LDADD = zbar/libzbar.la
# automake bug in "monolithic mode"?
CLEANFILES += zbarcam/.libs/zbarcam

if WIN32
zbarcam_zbarcam_SOURCES += zbarcam/zbarcam.rc
zbarcam_zbarcam_LDADD += zbarcam/zbarcam-rc.o
endif
