lib_LTLIBRARIES += qt/libzbarqt.la
qt_libzbarqt_la_CPPFLAGS = -Iqt $(QT_CFLAGS) $(AM_CPPFLAGS)
qt_libzbarqt_la_LDFLAGS = -version-info $(ZQT_LIB_VERSION) $(AM_LDFLAGS)
qt_libzbarqt_la_LIBADD = $(QT_LIBS) zbar/libzbar.la $(AM_LIBADD)

qt_libzbarqt_la_SOURCES = qt/QZBar.cpp qt/QZBarThread.h qt/QZBarThread.cpp
nodist_qt_libzbarqt_la_SOURCES = qt/moc_QZBar.cpp qt/moc_QZBarThread.cpp
BUILT_SOURCES += $(nodist_qt_libzbarqt_la_SOURCES)
DISTCLEANFILES += $(nodist_qt_libzbarqt_la_SOURCES)

qt/moc_%.cpp: qt/%.h
	$(MOC) $(qt_libzbarqt_la_CPPFLAGS) $< -o $@

qt/moc_%.cpp: include/zbar/%.h
	$(MOC) $(qt_libzbarqt_la_CPPFLAGS) $< -o $@
