lib_LTLIBRARIES += zebra/libzebra.la
zebra_libzebra_la_SOURCES = \
    zebra/img_walker.c zebra/scanner.c \
    zebra/decoder.h zebra/decoder.c \
    zebra/ean.h zebra/ean.c \
    zebra/code128.h zebra/code128.c
zebra_libzebra_la_LDFLAGS = -version-info $(LIB_VERSION)
