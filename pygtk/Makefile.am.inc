pyexec_LTLIBRARIES = pygtk/zebrapygtk.la
pygtk_zebrapygtk_la_CPPFLAGS = \
    $(GTK_CFLAGS) $(PYTHON_CFLAGS) $(PYGTK_CFLAGS) $(AM_CPPFLAGS)
pygtk_zebrapygtk_la_LDFLAGS = \
    -module -avoid-version -export-symbols-regex initzebrapygtk
pygtk_zebrapygtk_la_LIBADD = \
    $(PYTHON_LIBS) $(PYGTK_LIBS) gtk/libzebragtk.la $(AM_LIBADD)

pygtk_zebrapygtk_la_SOURCES = pygtk/zebrapygtkmodule.c pygtk/zebrapygtk.c
BUILT_SOURCES += pygtk/zebrapygtk.c pygtk/zebrapygtk.defs
EXTRA_DIST += pygtk/zebrapygtk.override pygtk/zebrapygtk.c pygtk/zebrapygtk.defs

pygtk/zebrapygtk.defs: include/zebra/zebragtk.h
	$(PYTHON) $(PYGTK_H2DEF) $< > $@

pygtk/%.c: pygtk/%.defs $(srcdir)/pygtk/zebrapygtk.override
	$(PYGTK_CODEGEN) --prefix zebrapygtk \
	    --override $(srcdir)/pygtk/zebrapygtk.override $< > $@
