lib_LTLIBRARIES += qt/libzebraqt.la
qt_libzebraqt_la_CPPFLAGS = -Iqt $(QT_CFLAGS) $(AM_CPPFLAGS)
qt_libzebraqt_la_LDFLAGS = -version-info $(ZQT_LIB_VERSION) $(AM_LDFLAGS)
qt_libzebraqt_la_LIBADD = $(QT_LIBS) zebra/libzebra.la $(AM_LIBADD)

qt_libzebraqt_la_SOURCES = qt/QZebra.cpp qt/moc_QZebra.cpp \
    qt/QZebraThread.h qt/QZebraThread.cpp qt/moc_QZebraThread.cpp

BUILT_SOURCES += qt/moc_QZebra.cpp qt/moc_QZebraThread.cpp

qt/moc_%.cpp: qt/%.h
	$(MOC) $(qt_libzebraqt_la_CPPFLAGS) $< -o $@

qt/moc_%.cpp: include/zebra/%.h
	$(MOC) $(qt_libzebraqt_la_CPPFLAGS) $< -o $@
