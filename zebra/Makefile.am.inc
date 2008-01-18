lib_LTLIBRARIES += zebra/libzebra.la
zebra_libzebra_la_SOURCES = \
    zebra/error.h zebra/error.c zebra/symbol.h zebra/symbol.c \
    zebra/image.h zebra/image.c zebra/convert.c \
    zebra/window.h zebra/window.c zebra/video.h zebra/video.c \
    zebra/processor.h zebra/processor.c \
    zebra/img_scanner.c zebra/scanner.c \
    zebra/decoder.h zebra/decoder.c \
    zebra/ean.h zebra/ean.c \
    zebra/code128.h zebra/code128.c

if HAVE_V4L1
zebra_libzebra_la_SOURCES += zebra/v4l1.c
if HAVE_V4L2
zebra_libzebra_la_SOURCES += zebra/v4l2.c
endif
endif

if HAVE_X
zebra_libzebra_la_SOURCES += zebra/x.c zebra/ximage.c
if HAVE_XV
zebra_libzebra_la_SOURCES += zebra/xv.c
endif
endif

zebra_libzebra_la_LDFLAGS = -version-info $(LIB_VERSION)
