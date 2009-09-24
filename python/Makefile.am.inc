pyexec_LTLIBRARIES += python/zbar.la
python_zbar_la_CPPFLAGS = $(PYTHON_CFLAGS) $(AM_CPPFLAGS)
python_zbar_la_LDFLAGS = -shared -module -avoid-version -export-dynamic \
    -export-symbols-regex initzbar
python_zbar_la_LIBADD = $(PYTHON_LIBS) zbar/libzbar.la $(AM_LIBADD)

python_zbar_la_SOURCES = python/zbarmodule.c python/zbarmodule.h \
    python/enum.c python/exception.c python/symbol.c python/symbolset.c \
    python/symboliter.c python/image.c \
    python/processor.c python/imagescanner.c python/decoder.c python/scanner.c

EXTRA_DIST += python/test/barcode.png python/test/test_zbar.py
