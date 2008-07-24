lib_LTLIBRARIES += zebra/libzebra.la
zebra_libzebra_la_CPPFLAGS = -I$(srcdir)/zebra $(AM_CPPFLAGS)
zebra_libzebra_la_LDFLAGS = -version-info $(LIB_VERSION) \
    -export-symbols-regex "^(zebra|_zebra.*_error)_.*" $(AM_LDFLAGS)
zebra_libzebra_la_LIBADD =

zebra_libzebra_la_SOURCES = zebra/debug.h zebra/config.c \
    zebra/error.h zebra/error.c zebra/symbol.h zebra/symbol.c \
    zebra/image.h zebra/image.c zebra/convert.c zebra/logo.c \
    zebra/window.h zebra/window.c zebra/video.h zebra/video.c \
    zebra/processor.h zebra/processor.c \
    zebra/img_scanner.c zebra/scanner.c \
    zebra/decoder.h zebra/decoder.c

# FIXME put configure switches on these
zebra_libzebra_la_SOURCES += zebra/decoder/ean.h zebra/decoder/ean.c
zebra_libzebra_la_SOURCES += zebra/decoder/code128.h zebra/decoder/code128.c
zebra_libzebra_la_SOURCES += zebra/decoder/code39.h zebra/decoder/code39.c
zebra_libzebra_la_SOURCES += zebra/decoder/i25.h zebra/decoder/i25.c

if HAVE_V4L1
zebra_libzebra_la_SOURCES += zebra/video/v4l1.c
if HAVE_V4L2
zebra_libzebra_la_SOURCES += zebra/video/v4l2.c
endif
else
zebra_libzebra_la_SOURCES += zebra/video/null.c
endif

if HAVE_X
zebra_libzebra_la_SOURCES += zebra/window/x.c zebra/window/ximage.c
zebra_libzebra_la_CPPFLAGS += $(X_CFLAGS)
zebra_libzebra_la_LDFLAGS += $(X_LIBS)
zebra_libzebra_la_LIBADD += $(X_PRE_LIBS) -lX11 $(X_EXTRA_LIBS)
if HAVE_XV
zebra_libzebra_la_SOURCES += zebra/window/xv.c
zebra_libzebra_la_LIBADD += $(XV_LIBS)
endif
else
zebra_libzebra_la_SOURCES += zebra/window/null.c
endif

zebra_libzebra_la_LDFLAGS += $(AM_LDFLAGS)
zebra_libzebra_la_LIBADD += $(AM_LIBADD)
