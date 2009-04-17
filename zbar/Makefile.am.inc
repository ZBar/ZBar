lib_LTLIBRARIES += zbar/libzbar.la
zbar_libzbar_la_CPPFLAGS = -I$(srcdir)/zbar $(AM_CPPFLAGS)
zbar_libzbar_la_LDFLAGS = -no-undefined -version-info $(LIB_VERSION) \
    -export-symbols-regex "^(zbar|_zbar.*_error)_.*" $(AM_LDFLAGS)
zbar_libzbar_la_LIBADD =

zbar_libzbar_la_SOURCES = zbar/debug.h zbar/config.c \
    zbar/error.h zbar/error.c zbar/symbol.h zbar/symbol.c \
    zbar/image.h zbar/image.c zbar/convert.c zbar/logo.c \
    zbar/window.h zbar/window.c zbar/video.h zbar/video.c \
    zbar/img_scanner.c zbar/scanner.c \
    zbar/decoder.h zbar/decoder.c

if HAVE_POLL
zbar_libzbar_la_SOURCES += zbar/processor.h zbar/processor.c
endif

if ENABLE_EAN
zbar_libzbar_la_SOURCES += zbar/decoder/ean.h zbar/decoder/ean.c
endif
if ENABLE_CODE128
zbar_libzbar_la_SOURCES += zbar/decoder/code128.h zbar/decoder/code128.c
endif
if ENABLE_CODE39
zbar_libzbar_la_SOURCES += zbar/decoder/code39.h zbar/decoder/code39.c
endif
if ENABLE_I25
zbar_libzbar_la_SOURCES += zbar/decoder/i25.h zbar/decoder/i25.c
endif
if ENABLE_PDF417
zbar_libzbar_la_SOURCES += zbar/decoder/pdf417.h zbar/decoder/pdf417.c \
    zbar/decoder/pdf417_hash.h
endif

if HAVE_V4L1
zbar_libzbar_la_SOURCES += zbar/video/v4l1.c
if HAVE_V4L2
zbar_libzbar_la_SOURCES += zbar/video/v4l2.c
endif
else
zbar_libzbar_la_SOURCES += zbar/video/null.c
endif

if HAVE_JPEG
zbar_libzbar_la_SOURCES += zbar/jpeg.c
endif

if HAVE_X
zbar_libzbar_la_SOURCES += zbar/window/x.c zbar/window/ximage.c
zbar_libzbar_la_CPPFLAGS += $(X_CFLAGS)
zbar_libzbar_la_LDFLAGS += $(X_LIBS)
zbar_libzbar_la_LIBADD += $(X_PRE_LIBS) -lX11 $(X_EXTRA_LIBS)
if HAVE_XV
zbar_libzbar_la_SOURCES += zbar/window/xv.c
zbar_libzbar_la_LIBADD += $(XV_LIBS)
endif
else
zbar_libzbar_la_SOURCES += zbar/window/null.c
endif

zbar_libzbar_la_LDFLAGS += $(AM_LDFLAGS)
zbar_libzbar_la_LIBADD += $(AM_LIBADD)
