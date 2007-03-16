lib_LTLIBRARIES = zebra/libzebra.la
zebra_libzebra_la_SOURCES = zebra/scanner.c zebra/decoder.c zebra/img_walker.c
zebra_libzebra_la_LDFLAGS = -version-info $(LIB_VERSION)
