# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.61])
AC_INIT([zebra], [0.2], [spadix@users.sourceforge.net])
AM_INIT_AUTOMAKE([1.10 -Wall -Werror foreign subdir-objects std-options dist-bzip2])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_SRCDIR(zebra/scanner.c)

# update these before each release (along w/pacakge version above)
#   LIB_VERSION update instructions copied from libtool docs:
#   library version follows the form current:revision:age
#   - If the library source code has changed at all since the last update,
#     then increment revision (c:r:a becomes c:r+1:a).
#   - If any interfaces have been added, removed, or changed,
#     increment current, and set revision to 0.
#   - If any interfaces have been added since the last public release,
#     then increment age.
#   - If any interfaces have been removed since the last public release,
#     then set age to 0. 
AC_SUBST([LIB_VERSION], [2:0:2])
AC_SUBST([RELDATE], [2007-05-16])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_ARG_VAR([XMLTO], [location of xmlto, used for optional \
documentation generation])
AC_ARG_VAR([XMLTOFLAGS], [additional arguments for xmlto])
AC_CHECK_TOOL([XMLTO], [xmlto], [:])

AC_ARG_WITH([imagemagick],
  [AS_HELP_STRING([--without-imagemagick],
    [disable support for scanning images using ImageMagick])],
  [],
  [with_imagemagick=yes])

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
AM_CONDITIONAL([HAVE_MAGICK], [test "x$MAGICKXX_CONFIG" != "x:"])
AC_SUBST([MAGICK_CPPFLAGS], [`$MAGICKXX_CONFIG --cppflags`])
AC_SUBST([MAGICK_CXXFLAGS], [`$MAGICKXX_CONFIG --cxxflags`])
AC_SUBST([MAGICK_LDFLAGS], [`$MAGICKXX_CONFIG --ldflags`])
AC_SUBST([MAGICK_LIBS], [`$MAGICKXX_CONFIG --libs`])


AC_ARG_ENABLE([video],
  [AS_HELP_STRING([--disable-video],
    [exclude video scanner application])],
  [],
  [enable_video=yes])

AC_CHECK_HEADER([linux/videodev.h],
  [],
  [AS_IF([test "x$enable_video" != "xno"],
    [AC_MSG_FAILURE([test for video support failed!
rebuild your kernel to include video4linux support or
configure --disable-video to skip building programs that use video.])]
)])


AC_ARG_WITH([sdl],
  [AS_HELP_STRING([--without-sdl],
    [disable support for live video display using SDL])],
  [],
  [with_sdl=yes])

AC_ARG_VAR([SDL_CONFIG], [full path to sdl-config program])
AC_CHECK_TOOL([SDL_CONFIG], [sdl-config], [:])
AS_IF([test "x$enable_video" = "xno"],
    [SDL_CONFIG=":"],
  [test "x$with_sdl" != "xno" && \
   test "x$SDL_CONFIG" = "x:"],
    [AC_MSG_FAILURE([test for SDL failed!
install SDL or configure --without-sdl
to skip building programs that use SDL])]
)
AM_CONDITIONAL([HAVE_SDL], [test "x$SDL_CONFIG" != "x:"])
AC_SUBST([SDL_CPPFLAGS], [`$SDL_CONFIG --cflags`])
AC_SUBST([SDL_LIBS], [`$SDL_CONFIG --libs`])

AC_PATH_X

AC_HEADER_ASSERT
AC_CHECK_HEADERS([fcntl.h inttypes.h stdlib.h string.h sys/ioctl.h sys/time.h])

AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T
AC_TYPE_UINTPTR_T

AC_C_CONST
AC_C_INLINE

AC_FUNC_MMAP
AC_CHECK_FUNCS([memset atexit setenv])

# avoid doc rebuilds unless revision info changes
AC_CONFIG_COMMANDS([doc/version.xml],
  [AS_IF([test -f doc/version.xml && \
          ! echo $VERSION | diff doc/version.xml - >/dev/null 2>&1 || \
          ! echo $VERSION | diff $srcdir/doc/version.xml - >/dev/null 2>&1 ],
    [echo "writing new doc/version.xml" ; echo $VERSION > doc/version.xml ])],
  [VERSION=$VERSION]
)
AC_CONFIG_COMMANDS([doc/reldate.xml],
  [AS_IF([test -f doc/reldate.xml && \
          ! echo $RELDATE | diff doc/reldate.xml - >/dev/null 2>&1 || \
          ! echo $RELDATE | diff $srcdir/doc/reldate.xml - >/dev/null 2>&1 ],
    [echo "writing new doc/reldate.xml" ; echo $RELDATE > doc/reldate.xml ])],
  [RELDATE=$RELDATE]
)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo ""
echo "please verify that the detected configuration matches your expectations:"
echo "------------------------------------------------------------------------"
echo "v4l               --enable-video=$enable_video"
echo "SDL               --with-sdl=$with_sdl"
AS_IF([test "x$enable_video" != "xyes" || test "x$with_sdl" != "xyes"],
  [echo "       => zebracam video scanner will *NOT* be built"],
  [echo "       => zebracam video scanner will be built"])
echo ""
echo "Magick++          --with-imagemagick=$with_imagemagick"
AS_IF([test "x$with_imagemagick" != "xyes"],
  [echo "       => the zebraimg program will *NOT* be built"],
  [echo "       => zebraimg image file scanner will be built"])
echo ""
