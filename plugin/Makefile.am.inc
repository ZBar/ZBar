lib_LTLIBRARIES += plugin/libzbarplugin.la
plugin_libzbarplugin_la_SOURCES = \
   plugin/plugin.c
plugin_libzbarplugin_la_CPPFLAGS = $(MOZILLA_CFLAGS) $(AM_CPPFLAGS)
plugin_libzbarplugin_la_LDFLAGS = $(MOZILLA_LIBS) $(AM_LDFLAGS)
