dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT([zbar], [0.7], [spadix@users.sourceforge.net])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)
AM_INIT_AUTOMAKE([1.10 -Wall -Werror foreign subdir-objects std-options dist-bzip2])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_SRCDIR(zbar/scanner.c)
LT_PREREQ([2.2])
LT_INIT([dlopen win32-dll])

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
AC_SUBST([LIB_VERSION], [0:0:0])
AC_SUBST([RELDATE], [2009-04-21])

dnl widget libraries use their own versioning.
dnl NB pygtk wrapper is *unversioned*
AC_SUBST([ZGTK_LIB_VERSION], [0:0:0])
AC_SUBST([ZQT_LIB_VERSION], [0:0:0])

AC_DEFINE_UNQUOTED([ZBAR_VERSION_MAJOR],
  [[`echo "$PACKAGE_VERSION" | sed -e 's/\..*$//'`]],
  [Program major version (before the '.') as a number])
AC_DEFINE_UNQUOTED([ZBAR_VERSION_MINOR],
  [[`echo "$PACKAGE_VERSION" | sed -e 's/^[^.]*\.\([^.]*\)$/\1/'`]],
  [Program minor version (after '.') as a number])

dnl programs

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX

PKG_PROG_PKG_CONFIG

AC_ARG_VAR([XMLTO], [location of xmlto, used for optional \
documentation generation])
AC_ARG_VAR([XMLTOFLAGS], [additional arguments for xmlto])
AC_CHECK_PROGS([XMLTO], [xmlto], [:])

dnl symbologies

AC_ARG_ENABLE([codes],
  [AS_HELP_STRING([--enable-codes=SYMS],
    [select symbologies to compile [default=ean,i25,code39,code128]])],
  [],
  [enable_codes="ean,code39,code128,i25"]) dnl pdf417

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
ZBAR_CHK_CODE([code128], [Code 128 symbology])
ZBAR_CHK_CODE([code39], [Code 39 symbology])
ZBAR_CHK_CODE([pdf417], [PDF417 symbology])
ZBAR_CHK_CODE([i25], [Interleaved 2 of 5 symbology])

dnl libraries

AC_SEARCH_LIBS([clock_gettime], [rt])

dnl poll support
dnl without poll() we exclude processor layer

have_poll="no"
AC_CHECK_HEADERS([poll.h], [have_poll="yes"])
AM_CONDITIONAL([HAVE_POLL], [test "x$have_poll" = "xyes"])

dnl pthreads
dnl FIXME this doesn't port well, integrate something like this:
dnl     http://autoconf-archive.cryp.to/acx_pthread.html

AC_ARG_ENABLE([pthread],
  [AS_HELP_STRING([--disable-pthread],
    [omit support for threaded applications])],
  [],
  [enable_pthread="yes"])

AS_IF([test "x$enable_pthread" != "xno"],
  [AC_CHECK_HEADERS([pthread.h], [],
     [AC_MSG_FAILURE([test for pthread support failed!
configure --disable-pthread to skip threaded support.])])
   AC_CHECK_LIB([pthread], [pthread_create], [],
     [AC_MSG_FAILURE([unable to link against -lpthread, although you
appear to have pthread.h? set LDFLAGS and/or LIBS to help the linker,
or configure --disable-pthread to skip threaded support.])])
])

dnl video
AC_ARG_ENABLE([video],
  [AS_HELP_STRING([--disable-video],
    [exclude video scanner features])],
  [],
  [enable_video="yes"])

with_video="no"
AS_IF([test "x$enable_video" != "xno"],
  [AC_CHECK_HEADERS([linux/videodev.h], [with_video="v4l1"],
     [AC_MSG_FAILURE([test for video support failed!
rebuild your kernel to include video4linux support or
configure --disable-video to skip building video support.])])
   AC_CHECK_HEADERS([linux/videodev2.h], [with_video="v4l2"],
     [AC_MSG_WARN([v4l2 API not detected, upgrade your kernel!])])
])
AM_CONDITIONAL([HAVE_VIDEO], [test "x$enable_video" != "xno"])
AM_CONDITIONAL([HAVE_V4L1],
  [test "x$with_video" = "xv4l1" || test "x$with_video" = "xv4l2"])
AM_CONDITIONAL([HAVE_V4L2], [test "x$with_video" = "xv4l2"])

dnl X
AC_PATH_XTRA
AH_BOTTOM([#ifndef X_DISPLAY_MISSING
# define HAVE_X
#endif
])
AS_IF([test "x$no_x" != "xyes"], [with_x="yes"], [with_x="no"])
AM_CONDITIONAL([HAVE_X], [test "x$with_x" = "xyes"])

AC_ARG_VAR([XSHM_LIBS], [linker flags for X shared memory extension])
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

dnl ImageMagick
AC_ARG_WITH([imagemagick],
  [AS_HELP_STRING([--without-imagemagick],
    [disable support for scanning images using ImageMagick])],
  [],
  [with_imagemagick="yes"])

AS_IF([test "x$with_imagemagick" != "xno"],
  [PKG_CHECK_MODULES([MAGICK], [ImageMagick++ >= 6.2.6])
   MAGICK_VERSION=`$PKG_CONFIG ImageMagick++ --modversion`
   AC_MSG_NOTICE([using ImageMagick version $MAGICK_VERSION])

dnl double check ImageMagick install (bug #2582232)
   AC_LANG_PUSH([C++])
   CPPFLAGS_save="$CPPFLAGS"
   CPPFLAGS="$CPPFLAGS $MAGICK_CFLAGS"
   AC_CHECK_HEADER([Magick++.h], [],
     [AC_MSG_FAILURE([ImageMagick++ package found but Magick++.h not installed?!
this is a problem with your ImageMagick install,
please resolve the inconsistency and try again...])])
   CPPFLAGS="$CPPFLAGS_save"
   AC_LANG_POP([C++])])

AM_CONDITIONAL([HAVE_MAGICK], [test "x$with_imagemagick" = "xyes"])

dnl Mozilla NPAPI
AC_ARG_WITH([npapi],
  [AS_HELP_STRING([--with-npapi],
    [enable support for Firefox/Mozilla/OpenOffice NPAPI plugin])],
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
dnl FIXME now should check that #include <Python.h> compiles

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


dnl header files

dnl FIXME switches for shm, mmap
AC_HEADER_ASSERT
AC_CHECK_HEADERS([fcntl.h inttypes.h stdlib.h string.h unistd.h \
  sys/ioctl.h sys/time.h sys/times.h sys/ipc.h sys/shm.h sys/mman.h])

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
zbar.pc
zbar-gtk.pc
zbar-qt.pc
zbar.spec
doc/doxygen.conf
])
AC_OUTPUT

dnl summary log

echo ""
echo "please verify that the detected configuration matches your expectations:"
echo "------------------------------------------------------------------------"
echo "X                 --with-x=$with_x"
echo "pthreads          --enable-pthread=$enable_pthread"
echo "v4l               --enable-video=$enable_video"
AS_IF([test "x$enable_video" != "xyes"],
  [echo "        => zbarcam video scanner will *NOT* be built"])
echo "jpeg              --with-jpeg=$with_jpeg"
AS_IF([test "x$with_jpeg" != "xyes"],
  [echo "        => JPEG image conversions will *NOT* be supported"])
echo "Magick++          --with-imagemagick=$with_imagemagick"
AS_IF([test "x$with_imagemagick" != "xyes"],
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
#echo "NPAPI Plugin     --with-npapi=$with_npapi"
#AS_IF([test "x$with_mozilla" != "xyes"],
#  [echo "       => the Mozilla/Firefox/OpenOffice plugin will *NOT* be built"])
echo ""
