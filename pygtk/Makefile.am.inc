pyexec_LTLIBRARIES += pygtk/zbarpygtk.la
pygtk_zbarpygtk_la_CPPFLAGS = \
    $(GTK_CFLAGS) $(PYTHON_CFLAGS) $(PYGTK_CFLAGS) $(AM_CPPFLAGS)
pygtk_zbarpygtk_la_LDFLAGS = -shared -module -avoid-version -export-dynamic \
    -export-symbols-regex initzbarpygtk
pygtk_zbarpygtk_la_LIBADD = \
    $(PYTHON_LIBS) $(PYGTK_LIBS) gtk/libzbargtk.la $(AM_LIBADD)

dist_pygtk_zbarpygtk_la_SOURCES = pygtk/zbarpygtkmodule.c
nodist_pygtk_zbarpygtk_la_SOURCES = pygtk/zbarpygtk.c
BUILT_SOURCES += pygtk/zbarpygtk.c pygtk/zbarpygtk.defs
CLEANFILES += pygtk/zbarpygtk.c pygtk/zbarpygtk.defs
EXTRA_DIST += pygtk/zbarpygtk.override

# FIXME ugly hack to fixup new name... now non-standard?
pygtk/zbarpygtk.defs: include/zbar/zbargtk.h
	$(PYTHON) $(PYGTK_H2DEF) $< | \
	$(SED) -e 's/Z_TYPE_BAR_/ZBAR_TYPE_/' > $@

pygtk/%.c: pygtk/%.defs $(srcdir)/pygtk/zbarpygtk.override
	$(PYGTK_CODEGEN) --prefix zbarpygtk \
            --register $(PYGTK_DEFS)/gdk-types.defs \
	    --override $(srcdir)/pygtk/zbarpygtk.override $< > $@
