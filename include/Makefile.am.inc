zincludedir = $(includedir)/zebra
include_HEADERS = include/zebra.h
zinclude_HEADERS = include/zebra/Scanner.h include/zebra/Decoder.h \
    include/zebra/Exception.h include/zebra/Symbol.h include/zebra/Image.h \
    include/zebra/ImageScanner.h include/zebra/Video.h include/zebra/Window.h \
    include/zebra/Processor.h

if HAVE_GTK
zinclude_HEADERS += include/zebra/zebragtk.h
endif
if HAVE_QT
zinclude_HEADERS += include/zebra/QZebra.h include/zebra/QZebraImage.h
endif
