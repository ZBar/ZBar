lib_LTLIBRARIES += gtk/libzbargtk.la
gtk_libzbargtk_la_CPPFLAGS = -Igtk $(GTK_CFLAGS) $(AM_CPPFLAGS)
gtk_libzbargtk_la_LDFLAGS = -version-info $(ZGTK_LIB_VERSION) \
    -export-symbols-regex "^zbar_gtk_.*" $(AM_LDFLAGS)
gtk_libzbargtk_la_LIBADD = $(GTK_LIBS) zbar/libzbar.la $(AM_LIBADD)

dist_gtk_libzbargtk_la_SOURCES = gtk/zbargtk.c gtk/zbargtkprivate.h
nodist_gtk_libzbargtk_la_SOURCES = gtk/zbarmarshal.c gtk/zbarmarshal.h
BUILT_SOURCES += gtk/zbarmarshal.c gtk/zbarmarshal.h
CLEANFILES += gtk/zbarmarshal.c gtk/zbarmarshal.h
EXTRA_DIST += gtk/zbarmarshal.list

%.h: %.list
	$(GLIB_GENMARSHAL) --g-fatal-warnings --prefix=zbar_marshal \
	    --header $^ > $@

%.c: %.list
	$(GLIB_GENMARSHAL) --g-fatal-warnings --prefix=zbar_marshal \
	    --body $^ > $@
