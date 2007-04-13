lib_LTLIBRARIES = zebra/libzebra.la
zebra_libzebra_la_SOURCES = \
    zebra/img_walker.c zebra/scanner.c \
    zebra/decoder.c zebra/ean.c
zebra_libzebra_la_LDFLAGS = -version-info $(LIB_VERSION)
