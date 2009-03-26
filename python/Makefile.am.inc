pyexec_LTLIBRARIES += python/zebra.la
python_zebra_la_CPPFLAGS = $(PYTHON_CFLAGS) $(AM_CPPFLAGS)
python_zebra_la_LDFLAGS = -shared -module -avoid-version -export-dynamic \
    -export-symbols-regex initzebra
python_zebra_la_LIBADD = $(PYTHON_LIBS) zebra/libzebra.la $(AM_LIBADD)

python_zebra_la_SOURCES = python/zebramodule.c python/zebramodule.h \
    python/enum.c python/exception.c python/symbol.c python/image.c \
    python/processor.c python/imagescanner.c python/decoder.c python/scanner.c
