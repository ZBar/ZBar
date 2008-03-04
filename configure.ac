dnl Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT([zebra], [0.3], [spadix@users.sourceforge.net])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_MACRO_DIR(config)
AM_INIT_AUTOMAKE([1.10 -Wall -Werror foreign subdir-objects std-options dist-bzip2])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_SRCDIR(zebra/scanner.c)

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
AC_SUBST([LIB_VERSION], [3:0:0])
AC_SUBST([RELDATE], [2008-02-25])

AC_DEFINE_UNQUOTED([ZEBRA_VERSION_MAJOR],
  [[`echo "$PACKAGE_VERSION" | sed -e 's/\..*$//'`]],
  [Program major version (before the '.') as a number])
AC_DEFINE_UNQUOTED([ZEBRA_VERSION_MINOR],
  [[`echo "$PACKAGE_VERSION" | sed -e 's/^[^.]*\.\([^.]*\)$/\1/'`]],
  [Program minor version (after '.') as a number])

dnl programs

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_ARG_VAR([XMLTO], [location of xmlto, used for optional \
documentation generation])
AC_ARG_VAR([XMLTOFLAGS], [additional arguments for xmlto])
AC_CHECK_TOOL([XMLTO], [xmlto], [:])

AC_ARG_VAR([PKG_CONFIG], [full path to pkg-config utility])
AC_CHECK_TOOL([PKG_CONFIG], [pkg-config], [:])

dnl libraries

AC_SEARCH_LIBS([clock_gettime], [rt])

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
    [exclude video scanner application])],
  [],
  [enable_video="yes"])

with_video="no"
AS_IF([test "x$enable_video" != "xno"],
  [AC_CHECK_HEADERS([linux/videodev.h], [with_video="v4l1"],
     [AC_MSG_FAILURE([test for video support failed!
rebuild your kernel to include video4linux support or
configure --disable-video to skip building video support.])])
   AC_CHECK_HEADERS([linux/videodev2.h], [with_video="v4l2"],
     [AC_MSG_WARN([v4l2 API not detected, upgrade your kernel!])],
     [[#include <linux/types.h>
#include <linux/time.h>
#include <linux/videodev.h>
]])
])
AM_CONDITIONAL([HAVE_VIDEO], [test "x$enable_video" != "xno"])
AM_CONDITIONAL([HAVE_V4L1],
  [test "x$with_video" == "xv4l1" || test "x$with_video" == "xv4l2"])
AM_CONDITIONAL([HAVE_V4L2], [test "x$with_video" == "xv4l2"])

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


dnl ImageMagick
AC_ARG_WITH([imagemagick],
  [AS_HELP_STRING([--without-imagemagick],
    [disable support for scanning images using ImageMagick])],
  [],
  [with_imagemagick="yes"])

AC_ARG_VAR([MAGICKXX_CONFIG], [full path to Magick++-config program])
AC_CHECK_TOOL([MAGICKXX_CONFIG], [Magick++-config], [:])
AS_IF([test "x$with_imagemagick" != "xno" && \
       test "x$MAGICKXX_CONFIG" = "x:"],
  [AC_CHECK_TOOL([CONVERT], [convert])
   AS_IF([test "x$CONVERT" != "x" && \
          convert -version | grep ImageMagick >/dev/null 2>&1],
     [AC_MSG_FAILURE([test for Magick++ interface failed!
although ImageMagick programs appear to be installed?
make sure you install any ImageMagick development packages
provided by your distribution or configure --without-imagemagick
to skip building programs that use ImageMagick.])],
     [AC_MSG_FAILURE([test for ImageMagick failed!
install imagemagick or configure --without-imagemagick
to skip building programs that use ImageMagick.])]
)])

AS_IF([test "x$with_imagemagick" != "xno"],
  [AC_MSG_CHECKING([ImageMagick version])
   MAGICK_VERSION=`$MAGICKXX_CONFIG --version`
   AC_MSG_RESULT([$MAGICK_VERSION])
   AS_CASE([$MAGICK_VERSION],
     [[[0-5].* | 6.[0-1].* | 6.2.[0-5]]],
     [AC_MSG_FAILURE([test for ImageMagick version >= 6.2.6 failed!
upgrade ImageMagick or configure --without-imagemagick
to skip building programs that use ImageMagick.])])
])
    
AM_CONDITIONAL([HAVE_MAGICK], [test "x$with_imagemagick" = "xyes"])
AC_SUBST([MAGICK_CPPFLAGS], [`$MAGICKXX_CONFIG --cppflags`])
AC_SUBST([MAGICK_CXXFLAGS], [`$MAGICKXX_CONFIG --cxxflags`])
AC_SUBST([MAGICK_LDFLAGS], [`$MAGICKXX_CONFIG --ldflags`])
AC_SUBST([MAGICK_LIBS], [`$MAGICKXX_CONFIG --libs`])

AC_ARG_WITH([mozilla],
  [AS_HELP_STRING([--without-mozilla],
    [disable support for Firefox/Mozilla/OpenOffice NPAPI plugin])],
  [],
  [with_mozilla="no"])

AC_ARG_VAR([MOZILLA_CFLAGS],
  [C compiler flags for Firefox/Mozilla plugin (overrides pkg-config)])
AS_IF([test "x$MOZILLA_CFLAGS" = "x"],
  [MOZILLA_CFLAGS=`$PKG_CONFIG --cflags firefox-plugin`])

AC_ARG_VAR([MOZILLA_LIBS],
  [linker flags for Firefox/Mozilla plugin (overrides pkg-config)])
AS_IF([test "x$MOZILLA_LIBS" = "x"],
  [MOZILLA_LIBS=`$PKG_CONFIG --libs firefox-plugin`])

AS_IF([test "x$with_mozilla" = "xno"],
  [],
  [test "x$MOZILLA_CFLAGS" = "x" || \
   test "x$MOZILLA_LIBS" = "x"],
  [AC_MSG_FAILURE([test for Firefox/Mozilla plugin (NPAPI) interface failed!
  - install pkg-config and Firefox
  - *or* install Firefox or the Mozilla SDK and manually specify
      MOZILLA_CFLAGS and MOZILLA_LIBS
  - *or* configure --without-mozilla to skip building the plugin])]
)
AS_IF([test "x$with_mozilla" != "xno"],
  [AC_MSG_CHECKING([Mozilla version])
   MOZILLA_VERSION=`$PKG_CONFIG --modversion firefox-plugin`
   AC_MSG_RESULT([$MOZILLA_VERSION])
])

AM_CONDITIONAL([HAVE_MOZILLA], [test "x$with_mozilla" != "xno"])

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

AC_CONFIG_FILES([Makefile zebra.spec doc/doxygen.conf])
AC_OUTPUT

dnl summary log

echo ""
echo "please verify that the detected configuration matches your expectations:"
echo "------------------------------------------------------------------------"
echo "X                 --with-x=$with_x"
echo "v4l               --enable-video=$enable_video"
AS_IF([test "x$enable_video" != "xyes"],
  [echo "       => zebracam video scanner will *NOT* be built"],
  [echo "       => zebracam video scanner will be built"])
echo ""
echo "Magick++          --with-imagemagick=$with_imagemagick"
AS_IF([test "x$with_imagemagick" != "xyes"],
  [echo "       => the zebraimg file scanner will *NOT* be built"],
  [echo "       => zebraimg image file scanner will be built"])
echo ""
#echo "Mozilla Plugin    --with-mozilla=$with_mozilla"
#AS_IF([test "x$with_mozilla" != "xyes"],
#  [echo "       => the Mozilla/Firefox/OpenOffice plugin will *NOT* be built"],
#  [echo "       => the Mozilla/Firefox/OpenOffice plugin will be built"])
#echo ""
