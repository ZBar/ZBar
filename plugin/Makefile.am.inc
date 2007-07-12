lib_LTLIBRARIES += plugin/libzebraplugin.la
plugin_libzebraplugin_la_SOURCES = \
   plugin/plugin.c
plugin_libzebraplugin_la_CPPFLAGS = $(MOZILLA_CFLAGS) $(AM_CPPFLAGS)
plugin_libzebraplugin_la_LDFLAGS = $(MOZILLA_LIBS) $(AM_LDFLAGS)
