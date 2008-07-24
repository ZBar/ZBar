lib_LTLIBRARIES += gtk/libzebragtk.la
gtk_libzebragtk_la_CPPFLAGS = -Igtk $(GTK_CFLAGS) $(AM_CPPFLAGS)
gtk_libzebragtk_la_LDFLAGS = -version-info $(ZGTK_LIB_VERSION) \
    -export-symbols-regex "^zebra_gtk_.*" $(AM_LDFLAGS)
gtk_libzebragtk_la_LIBADD = $(GTK_LIBS) zebra/libzebra.la $(AM_LIBADD)

gtk_libzebragtk_la_SOURCES = gtk/zebragtk.c gtk/zebragtkprivate.h \
    gtk/zebramarshal.c gtk/zebramarshal.h
BUILT_SOURCES += gtk/zebramarshal.c gtk/zebramarshal.h
EXTRA_DIST += gtk/zebramarshal.list

%.h: %.list
	$(GLIB_GENMARSHAL) --g-fatal-warnings --prefix=zebra_marshal \
	    --header $^ > $@

%.c: %.list
	$(GLIB_GENMARSHAL) --g-fatal-warnings --prefix=zebra_marshal \
	    --body $^ > $@
