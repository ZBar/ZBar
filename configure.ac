dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT([zbar], [0.10], [spadix@users.sourceforge.net])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)
AM_INIT_AUTOMAKE([1.10 -Wall -Werror foreign subdir-objects std-options dist-bzip2])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_SRCDIR(zbar/scanner.c)
LT_PREREQ([2.2])
LT_INIT([dlopen win32-dll])
LT_LANG([Windows Resource])

dnl update these just before each release (along w/pacakge version above)
dnl   LIB_VERSION update instructions copied from libtool docs:
dnl   library version follows the form current:revision:age
dnl   - If the library source code has changed at all since the last update,
dnl     then increment revision (c:r:a becomes c:r+1:a).
dnl   - If any interfaces have been added, removed, or changed,
dnl     increment current, and set revision to 0.
dnl   - If any interfaces have been added since the last public release,
dnl     then increment age.
dnl   - If any interfaces have been removed since the last public release,
dnl     then set age to 0. 
AC_SUBST([LIB_VERSION], [2:0:2])
AC_SUBST([RELDATE], [2009-10-23])

dnl widget libraries use their own versioning.
dnl NB pygtk wrapper is *unversioned*
AC_SUBST([ZGTK_LIB_VERSION], [0:2:0])
AC_SUBST([ZQT_LIB_VERSION], [0:2:0])

AC_DEFINE_UNQUOTED([ZBAR_VERSION_MAJOR],
  [[`echo "$PACKAGE_VERSION" | sed -e 's/\..*$//'`]],
  [Program major version (before the '.') as a number])
AC_DEFINE_UNQUOTED([ZBAR_VERSION_MINOR],
  [[`echo "$PACKAGE_VERSION" | sed -e 's/^[^.]*\.\([^.]*\)$/\1/'`]],
  [Program minor version (after '.') as a number])

cur=`echo "$LIB_VERSION" | sed -e 's/:.*$//'`
age=`echo "$LIB_VERSION" | sed -e 's/^.*://'`
AC_DEFINE_UNQUOTED([LIB_VERSION_MAJOR], [[$(( $cur - $age ))]],
  [Library major version])
AC_DEFINE_UNQUOTED([LIB_VERSION_MINOR], [[$age]],
  [Library minor version])
AC_DEFINE_UNQUOTED([LIB_VERSION_REVISION],
  [[`echo "$LIB_VERSION" | sed -e 's/^[^:]*:\([^:]*\):.*$/\1/'`]],
  [Library revision])

AM_CPPFLAGS="-I$srcdir/include"
AM_CFLAGS="-Wall -Wno-parentheses -Werror"
AM_CXXFLAGS="$AM_CFLAGS"
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])

dnl windows build

AC_CANONICAL_HOST
case $host_os in
  *cygwin* | *mingw* | *uwin* | *djgpp* | *ems* )
    win32="yes"
    AC_DEFINE([_WIN32_WINNT], [0x0500], [Minimum Windows API version])
  ;;
  * )
    win32="no"
  ;;
esac
AM_CONDITIONAL([WIN32], [test "x$win32" = "xyes"])

dnl programs

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX

PKG_PROG_PKG_CONFIG

AC_ARG_VAR([XMLTO], [location of xmlto, used for optional \
documentation generation])
AC_ARG_VAR([XMLTOFLAGS], [additional arguments for xmlto])
AC_CHECK_PROGS([XMLTO], [xmlto], [:])

have_java="maybe"
AC_ARG_VAR([JAVA_HOME], [root location of JDK])
AS_IF([test "x$JAVA_HOME" = "x"],
  [JAVA_PATH="$PATH"],
  [JAVA_PATH="$JAVA_HOME/bin$PATH_SEPARATOR$PATH"])

AC_ARG_VAR([JAVAC], [location of Java language compiler])
AC_PATH_PROGS([JAVAC], [javac jikes ecj gcj], [:], [$JAVA_PATH])
AS_IF([test "x$JAVAC" = "x:"], [have_java="no"])

AC_ARG_VAR([JAVAH], [location of Java header generator])
AC_PATH_PROGS([JAVAH], [javah], [/bin/false], [$JAVA_PATH])

AC_ARG_VAR([JAR], [location of Java archive tool])
AC_PATH_PROGS([JAR], [jar], [:], [$JAVA_PATH])
AS_IF([test "x$JAR" = "x:"], [have_java="no"])

AC_ARG_VAR([JAVA], [location of Java application launcher])
AC_PATH_PROGS([JAVA], [java], [/bin/false], [$JAVA_PATH])

AC_ARG_VAR([CLASSPATH], [Java class path (include JUnit to run java tests)])
AS_IF([test "x$CLASSPATH" = "x"], [CLASSPATH="."])

dnl symbologies

AC_ARG_ENABLE([codes],
  [AS_HELP_STRING([--enable-codes=SYMS],
    [select symbologies to compile [default=ean,databar,code128,code93,code39,codabar,i25,qrcode]])],
  [],
  [enable_codes="ean,databar,code128,code93,code39,codabar,i25,qrcode"])

AC_DEFUN([ZBAR_CHK_CODE], [
  AC_MSG_CHECKING([whether to build $2])
  enable_$1="no"
  AH_TEMPLATE([ENABLE_]translit($1, a-z, A-Z),
    [whether to build support for $2])
  AS_CASE([$enable_codes],
    [*$1* | *all*],
    [enable_$1="yes"
     AC_DEFINE([ENABLE_]translit($1, a-z, A-Z), [1])
    ])
  AM_CONDITIONAL([ENABLE_]translit($1, a-z, A-Z),
    [test "x$enable_$1" = "xyes"])
  AC_MSG_RESULT([$enable_$1])
])dnl

ZBAR_CHK_CODE([ean], [EAN symbologies])
ZBAR_CHK_CODE([databar], [DataBar symbology])
ZBAR_CHK_CODE([code128], [Code 128 symbology])
ZBAR_CHK_CODE([code93], [Code 93 symbology])
ZBAR_CHK_CODE([code39], [Code 39 symbology])
ZBAR_CHK_CODE([codabar], [Codabar symbology])
ZBAR_CHK_CODE([i25], [Interleaved 2 of 5 symbology])
ZBAR_CHK_CODE([qrcode], [QR Code])
ZBAR_CHK_CODE([pdf417], [PDF417 symbology])

dnl libraries

AC_SEARCH_LIBS([clock_gettime], [rt])
AM_ICONV()

dnl poll support

AC_CHECK_HEADERS([poll.h], [have_poll="yes"], [have_poll="no"])
AM_CONDITIONAL([HAVE_POLL], [test "x$have_poll" = "xyes"])

dnl pthreads
dnl FIXME this doesn't port well, integrate something like this:
dnl     http://autoconf-archive.cryp.to/acx_pthread.html

AC_ARG_ENABLE([pthread],
  [AS_HELP_STRING([--disable-pthread],
    [omit support for threaded applications])],
  [],
  [AS_IF([test "x$win32" = "xno"],
   [enable_pthread="yes"],
   [enable_pthread="no"
])])

AS_IF([test "x$enable_pthread" != "xno"],
  [AC_CHECK_HEADERS([pthread.h], [],
     [AC_MSG_FAILURE([test for pthread support failed!
configure --disable-pthread to skip threaded support.])])
   AC_CHECK_LIB([pthread], [pthread_create], [],
     [AC_MSG_FAILURE([unable to link against -lpthread, although you
appear to have pthread.h? set LDFLAGS and/or LIBS to help the linker,
or configure --disable-pthread to skip threaded support.])])
   AC_DEFINE([__USE_UNIX98], [1], [used only for pthread debug attributes])
])

dnl video
AC_ARG_ENABLE([video],
  [AS_HELP_STRING([--disable-video],
    [exclude video scanner features])],
  [],
  [enable_video="yes"])

have_v4l1="no"
have_v4l2="no"
AS_IF([test "x$enable_video" = "xno"],
  [],
  [test "x$win32" = "xno"],
  [AC_CHECK_HEADERS([linux/videodev.h], [have_v4l1="yes"])
   AC_CHECK_HEADERS([linux/videodev2.h], [have_v4l2="yes"])
   AS_IF([test "x$have_v4l2" = "xno" && test "x$have_v4l1" = "xno"],
     [AC_MSG_FAILURE([test for video support failed!
rebuild your kernel to include video4linux support or
configure --disable-video to skip building video support.])],
     [test "x$have_v4l2" = "xno"],
     [AC_MSG_WARN([v4l2 API not detected, upgrade your kernel!])])],
  [AC_CHECK_HEADERS([vfw.h], [with_video="vfw"],
    [AC_MSG_FAILURE([test for VfW video support failed!
configure --disable-video to skip building video support.])])])

AM_CONDITIONAL([HAVE_VIDEO], [test "x$enable_video" != "xno"])
AM_CONDITIONAL([HAVE_V4L1], [test "x$have_v4l1" != "xno"])
AM_CONDITIONAL([HAVE_V4L2], [test "x$have_v4l2" != "xno"])

dnl X
AC_ARG_VAR([XSHM_LIBS], [linker flags for X shared memory extension])

AS_IF([test "x$win32" != "xno"],
  [have_x="no"],
  [AC_PATH_XTRA
   AH_BOTTOM([#ifndef X_DISPLAY_MISSING
# define HAVE_X
#endif
])])
AM_CONDITIONAL([HAVE_X], [test "x$have_x" = "xyes"])

AS_IF([test "x$XSHM_LIBS" = "x"], [XSHM_LIBS="-lXext"])
AC_ARG_WITH([xshm],
  [AS_HELP_STRING([--without-xshm],
    [disable support for X shared memory extension])],
  [],
  [with_xshm="check"])

AS_IF([test "x$with_xshm" != "xno"],
  [AC_CHECK_HEADERS([X11/extensions/XShm.h],
    [with_xshm="yes"],
    [AS_IF([test "x$with_xshm" = "xcheck"],
      [with_xshm="no"],
      [AC_MSG_FAILURE([test for X shared memory extension failed!
install the X shared memory extension, specify --x-includes or
configure --without-xshm to disable the extension])])],
    [[#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
]])
   AS_IF([test "x$with_xshm" != "xno"],
     [AC_CHECK_LIB([Xext], [XShmQueryVersion],
       [with_xshm="yes"],
       [AC_MSG_FAILURE([unable to find XShmQueryVersion in $XSHM_LIBS!
specify XSHM_LIBS or configure --without-xshm to disable the extension])],
       ["$X_LIBS" "$X_PRE_LIBS" -lX11 "$X_EXTRA_LIBS" "$XSHM_LIBS"])
   ])
])
AM_CONDITIONAL([HAVE_XSHM], [test "x$with_xshm" = "xyes"])

AC_ARG_VAR([XV_LIBS], [linker flags for XVideo extension])
AS_IF([test "x$XV_LIBS" = "x"], [XV_LIBS="-lXv"])
AC_ARG_WITH([xv],
  [AS_HELP_STRING([--without-xv],
    [disable support for XVideo extension])],
  [],
  [with_xv="check"])

AS_IF([test "x$with_xv" != "xno"],
  [AC_CHECK_HEADERS([X11/extensions/Xvlib.h],
    [with_xv="yes"],
    [AS_IF([test "x$with_xv" = "xcheck"],
      [with_xv="no"],
      [AC_MSG_FAILURE([test for XVideo extension failed!
install the XVideo extension, specify --x-includes or
configure --without-xv to disable the extension])])],
    [[#include <X11/Xlib.h>
]])
   AS_IF([test "x$with_xv" != "xno"],
     [AC_CHECK_LIB([Xv], [XvQueryExtension],
       [with_xv="yes"],
       [AC_MSG_FAILURE([unable to find XvQueryExtension in $XV_LIBS!
specify XV_LIBS or configure --without-xv to disable the extension])],
       ["$X_LIBS" "$X_PRE_LIBS" -lX11 "$X_EXTRA_LIBS" "$XV_LIBS"])
   ])
])
AM_CONDITIONAL([HAVE_XV], [test "x$with_xv" = "xyes"])

dnl libjpeg
AC_ARG_WITH([jpeg],
  [AS_HELP_STRING([--without-jpeg],
    [disable support for JPEG image conversions])],
  [],
  [with_jpeg="check"])

have_jpeg="maybe"
AS_IF([test "x$with_jpeg" != "xno"],
  [AC_CHECK_HEADERS([jpeglib.h], [], [have_jpeg="no"])
   AC_CHECK_HEADER([jerror.h], [], [have_jpeg="no"])
   AC_CHECK_LIB([jpeg], [jpeg_read_header], [], [have_jpeg="no"])
   AS_IF([test "x$have_jpeg" != "xno"],
     [with_jpeg="yes"],
     [test "x$with_jpeg" = "xyes"],
     [AC_MSG_FAILURE([unable to find libjpeg! ensure CFLAGS/LDFLAGS are
set appropriately or configure --without-jpeg])],
     [with_jpeg="no"])
])
AM_CONDITIONAL([HAVE_JPEG], [test "x$with_jpeg" = "xyes"])

dnl ImageMagick or GraphicsMagick
dnl disable both if IM is explicitly disabled to preserve old behavior

AC_ARG_WITH([imagemagick],
  [AS_HELP_STRING([--without-imagemagick],
    [disable support for scanning images with ImageMagick])],
  [],
  [with_imagemagick="check"])

AC_ARG_WITH([graphicsmagick],
  [AS_HELP_STRING([--with-graphicsmagick],
    [use GraphicsMagick alternative to ImageMagick])],
  [],
  [with_graphicsmagick="check"])

magick="UnknownMagick"
have_IM="maybe"
AS_IF([test "x$with_imagemagick" = "xno"], [],
      [test "x$with_imagemagick" = "xyes" || \
       test "x$with_graphicsmagick" != "xyes"],
  [looked_for="ImageMagick >= 6.2.6"
   PKG_CHECK_MODULES([MAGICK], [MagickWand >= 6.2.6],
     [MAGICK_VERSION=`$PKG_CONFIG MagickWand --modversion`],
     [dnl
dnl Wand is deprecated in favor of MagickWand,
dnl but the latter doesn't exist in older verisons (bug #2848437)
      saved_error=$MAGICK_PKG_ERRORS
      PKG_CHECK_MODULES([MAGICK], [Wand >= 6.2.6],
        [MAGICK_VERSION=`$PKG_CONFIG Wand --modversion`],
        [have_IM="no"])])
   AS_IF([test "x$have_IM" != "xno"],
     [magick="ImageMagick"
      AC_MSG_NOTICE([trying ImageMagick version $MAGICK_VERSION])
dnl double check ImageMagick install (bug #2582232)
      CPPFLAGS_save="$CPPFLAGS"
      CPPFLAGS="$CPPFLAGS $MAGICK_CFLAGS"
      AC_CHECK_HEADER([wand/MagickWand.h],
        [have_IM="yes"],
        [have_IM="broken"])
      CPPFLAGS="$CPPFLAGS_save"])])

have_GM="maybe"
AS_IF([test "x$have_IM" = "xyes"], [],
      [test "x$with_graphicsmagick" = "xno"], [],
      [test "x$with_graphicsmagick" = "xyes" || \
       test "x$with_imagemagick" = "xcheck"],
  [AS_IF([test "x$looked_for" = "x"],
     [looked_for="GraphicsMagick"],
     [looked_for="$looked_for or GraphicsMagick"])
   PKG_CHECK_MODULES([MAGICK], [GraphicsMagickWand],
     [have_GM="yes"
      magick="GraphicsMagick"
      MAGICK_VERSION=`$PKG_CONFIG GraphicsMagickWand --modversion`],
     [have_GM="no"
      AS_IF([test "x$saved_error" = "x"],
        [saved_error=$MAGICK_PKG_ERRORS])])])

dnl now that we have collected all the info abt what Magick is available
dnl let the user know what we will or can't do
AS_IF([test "x$have_IM" = "xbroken" && test "x$have_GM" = "xyes"],
  [AC_MSG_WARN([Your ImageMagick install is broken, using GraphicsMagick instead])])

AS_IF([test "x$have_IM" = "xyes" || test "x$have_GM" = "xyes"],
  [AC_MSG_NOTICE([using $magick version $MAGICK_VERSION])],
  [test "x$with_imagemagick" = "xno" && \
   test "x$with_graphicsmagick" != "xyes"],
  [AC_MSG_NOTICE([image scanning disabled -- zbarimg will *not* be built])],
  [test "x$have_IM" = "xbroken"],
  [AC_MSG_FAILURE([$magick package found but wand/MagickWand.h not installed?!
this is a problem with your $magick install, please try again after
resolving the inconsistency or installing GraphicsMagick alternative...])],
  [AC_MSG_FAILURE([dnl
Unable to find $looked_for:

$saved_error

* Ensure that you installed any "development" packages for ImageMagick or
  GraphicsMagick.
* Consider adjusting the PKG_CONFIG_PATH environment variable if you
  installed software in a non-standard prefix.
* You may set the environment variables MAGICK_CFLAGS and MAGICK_LIBS
  to avoid the need to call pkg-config.
  See the pkg-config man page for more details.
* To avoid using ImageMagick or GraphicsMagick altogether you may add the
  --without-imagemagick flag to the configure command; the zbarimg program
  will *not* be built.
])])

AS_IF([test "x$have_IM" = "xyes"],
  [AC_DEFINE([HAVE_IMAGEMAGICK], [1], [Define to 1 to use ImageMagick])],
  [test "x$have_GM" = "xyes"],
  [AC_DEFINE([HAVE_GRAPHICSMAGICK], [1], [Define to 1 to use GraphicsMagick])])
AM_CONDITIONAL([HAVE_MAGICK],
  [test "x$have_IM" = "xyes" || test "x$have_GM" = "xyes"])

dnl Mozilla NPAPI
AC_ARG_WITH([npapi],
  [AS_HELP_STRING([--with-npapi],
    [enable support for Firefox/Mozilla/OpenOffice plugin])],
  [],
  [with_npapi="no"])

AS_IF([test "x$with_npapi" != "xno"],
  [PKG_CHECK_MODULES([NPAPI], [firefox-plugin])
   NPAPI_VERSION=`$PKG_CONFIG firefox-plugin --modversion`
   AC_MSG_NOTICE([using firefox-plugin version $NPAPI_VERSION])])

AM_CONDITIONAL([HAVE_NPAPI], [test "x$with_npapi" = "xyes"])

dnl GTK
AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--without-gtk],
    [disable support for GTK+ widget])],
  [],
  [with_gtk="yes"])

AC_ARG_VAR([GLIB_GENMARSHAL], [full path to glib-genmarshal])

AS_IF([test "x$with_gtk" != "xno"],
  [PKG_CHECK_MODULES([GTK], [gtk+-2.0 gthread-2.0])
   GLIB_GENMARSHAL=`$PKG_CONFIG glib-2.0 --variable=glib_genmarshal`
   GTK_VERSION=`$PKG_CONFIG gtk+-2.0 --modversion`
   AC_MSG_NOTICE([using GTK+ version $GTK_VERSION])
   GTHREAD_VERSION=`$PKG_CONFIG gthread-2.0 --modversion`
   AC_MSG_NOTICE([using gthread-2.0 version $GTHREAD_VERSION])])

AM_CONDITIONAL([HAVE_GTK], [test "x$with_gtk" = "xyes"])

dnl PyGTK
AC_ARG_WITH([python],
  [AS_HELP_STRING([--without-python],
    [disable support for Python bindings])],
  [],
  [with_python="yes"])

AC_ARG_VAR([PYTHON_CONFIG], [full path to python-config program])
AC_ARG_VAR([PYTHON_CFLAGS], [compiler flags for building python extensions])
AC_ARG_VAR([PYTHON_LIBS], [linker flags for building python extensions])

AC_ARG_VAR([PYGTK_H2DEF], [full path to PyGTK h2def.py module])
AC_ARG_VAR([PYGTK_CODEGEN], [full path to pygtk-codegen program])
AC_ARG_VAR([PYGTK_DEFS], [directory where PyGTK definitions may be found])

AS_IF([test "x$with_python" != "xno"],
  [AM_PATH_PYTHON(2.3.5)
   AS_IF([test "x$PYTHON_CFLAGS" != "x"],
     [],
     [test "x$PYTHON_CONFIG" != "x" && test -x "$PYTHON_CONFIG"],
     [PYTHON_CFLAGS=`$PYTHON_CONFIG --cflags`],
     [test -x "$PYTHON-config"],
     [PYTHON_CFLAGS=`$PYTHON-config --cflags`],
     [PYTHON_CFLAGS=`$PYTHON -c 'import distutils.sysconfig as s; print " ".join(s.get_config_vars("CFLAGS")) + " -I"+s.get_python_inc() + " -I"+s.get_python_inc(plat_specific=True)'`])

dnl check that #include <Python.h> compiles (bug #3092663)
   CPPFLAGS_save="$CPPFLAGS"
   CPPFLAGS="$CPPFLAGS $PYTHON_CFLAGS"
   AC_CHECK_HEADER([Python.h], [], [AC_MSG_ERROR([dnl
Python module enabled, but unable to compile Python.h.
Install the development package for python-$am_cv_python_version, or configure
--without-python to disable the python bindings.dnl
])])
   CPPFLAGS="$CPPFLAGS_save"

   AS_IF([test "x$with_gtk" = "xyes"],
     [PKG_CHECK_MODULES([PYGTK], [pygtk-2.0])
      AC_CHECK_PROGS([PYGTK_CODEGEN], [pygtk-codegen-2.0 pygtk-codegen], [:])
      AS_IF([test "x$PYGTK_H2DEF" = "x"],
        [PYGTK_H2DEF=`$PKG_CONFIG pygtk-2.0 --variable=codegendir`/h2def.py
         AS_IF([test -f "$PYGTK_H2DEF"], [], [PYGTK_H2DEF=":"])])
      AS_IF([test "x$PYGTK_DEFS" = "x"],
        [PYGTK_DEFS=`$PKG_CONFIG pygtk-2.0 --variable=defsdir`])
   ])
])

AM_CONDITIONAL([HAVE_PYTHON], [test "x$with_python" = "xyes"])

dnl Qt
AC_ARG_WITH([qt],
  [AS_HELP_STRING([--without-qt],
    [disable support for Qt4 widget])],
  [],
  [with_qt="yes"])

AC_ARG_VAR([MOC], [full path to Qt moc program])

AS_IF([test "x$with_qt" != "xno"],
  [PKG_CHECK_MODULES([QT], [QtCore >= 4 QtGui >= 4])
   MOC=`$PKG_CONFIG QtGui --variable=moc_location`
   AC_MSG_NOTICE([using moc from $MOC])
   QT_VERSION=`$PKG_CONFIG QtGui --modversion`
   AC_MSG_NOTICE([using Qt version $QT_VERSION])])

AM_CONDITIONAL([HAVE_QT], [test "x$with_qt" = "xyes"])

dnl Java
AC_ARG_WITH([java],
  [AS_HELP_STRING([--without-java],
    [disable support for Java interface])],
  [],
  [with_java="check"])

AC_ARG_VAR([JAVA_CFLAGS], [compiler flags for building JNI extensions])
AS_IF([test "x$JAVA_CFLAGS" = "x" && test "x$JAVA_HOME" != "x"],
  [JAVA_CFLAGS="-I$JAVA_HOME/include"])

AS_IF([test "x$with_java" != "xno"],
  [CPPFLAGS_save="$CPPFLAGS"
   CPPFLAGS="$CPPFLAGS $JAVA_CFLAGS"
   AC_CHECK_HEADER([jni.h], [], [have_java="no"])
   CPPFLAGS="$CPPFLAGS_save"
   AS_IF([test "x$have_java" != "xno"],
     [with_java="yes"],
     [test "x$with_java" = "xyes"],
     [AC_MSG_FAILURE([unable to find Java JNI! ensure CFLAGS are set
appropriately or configure --without-java])],
     [with_java="no"])
])
AM_CONDITIONAL([HAVE_JAVA], [test "x$with_java" = "xyes"])


dnl header files

dnl FIXME switches for shm, mmap
AC_HEADER_ASSERT
AC_CHECK_HEADERS([errno.h fcntl.h features.h inttypes.h stdlib.h string.h \
  unistd.h sys/types.h sys/stat.h sys/ioctl.h sys/time.h sys/times.h \
  sys/ipc.h sys/shm.h sys/mman.h])

dnl types

AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_TYPE_UINTPTR_T

dnl compile characteristics

AC_C_CONST
AC_C_INLINE

dnl functions

AC_FUNC_MMAP
AC_CHECK_FUNCS([memset atexit setenv])

dnl output generation

dnl avoid doc rebuilds unless revision info changes
AC_CONFIG_COMMANDS([doc/version.xml],
  [AS_IF([test -f doc/version.xml && \
          ! echo $VERSION | diff doc/version.xml - >/dev/null 2>&1 || \
          ! echo $VERSION | diff $srcdir/doc/version.xml - >/dev/null 2>&1 ],
    [echo "writing new doc/version.xml" ; echo $VERSION > doc/version.xml ])],
  [VERSION="$VERSION"]
)
AC_CONFIG_COMMANDS([doc/reldate.xml],
  [AS_IF([test -f doc/reldate.xml && \
          ! echo $RELDATE | diff doc/reldate.xml - >/dev/null 2>&1 || \
          ! echo $RELDATE | diff $srcdir/doc/reldate.xml - >/dev/null 2>&1 ],
    [echo "writing new doc/reldate.xml" ; echo $RELDATE > doc/reldate.xml ])],
  [RELDATE="$RELDATE"]
)

AC_CONFIG_FILES([
Makefile
java/Makefile
zbar.pc
zbar-gtk.pc
zbar-qt.pc
doc/doxygen.conf
])
AC_OUTPUT

dnl summary log

echo ""
echo "please verify that the detected configuration matches your expectations:"
echo "------------------------------------------------------------------------"
echo "X                 --with-x=$have_x"
echo "pthreads          --enable-pthread=$enable_pthread"
echo "v4l               --enable-video=$enable_video"
AS_IF([test "x$enable_video" != "xyes"],
  [echo "        => zbarcam video scanner will *NOT* be built"])
echo "jpeg              --with-jpeg=$with_jpeg"
AS_IF([test "x$with_jpeg" != "xyes"],
  [echo "        => JPEG image conversions will *NOT* be supported"])
AS_IF([test "x$have_GM" = "xyes"],
  [echo "GraphicsMagick    --with-graphicsmagick=yes"],
  [echo "ImageMagick       --with-imagemagick=$with_imagemagick"])
AS_IF([test "x$have_IM" != "xyes" && test "x$have_GM" != "xyes"],
  [echo "        => the zbarimg file scanner will *NOT* be built"])
echo "Python            --with-python=$with_python"
echo "GTK+              --with-gtk=$with_gtk"
AS_IF([test "x$with_gtk" != "xyes"],
  [echo "        => the GTK+ widget will *NOT* be built"],
  [AS_IF([test "x$with_python" != "xyes"],
     [echo "        => the PyGTK widget wrapper will *NOT* be built"])])
echo "Qt4               --with-qt=$with_qt"
AS_IF([test "x$with_qt" != "xyes"],
  [echo "        => the Qt4 widget will *NOT* be built"])
echo "Java              --with-java=$with_java"
AS_IF([test "x$with_java" != "xyes"],
  [echo "        => the Java interface will *NOT* be built"])
#echo "NPAPI Plugin     --with-npapi=$with_npapi"
#AS_IF([test "x$with_mozilla" != "xyes"],
#  [echo "       => the Mozilla/Firefox/OpenOffice plugin will *NOT* be built"])
echo ""
