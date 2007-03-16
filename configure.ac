# Process this file with autoconf to produce a configure script.
AC_PREREQ([2.60])
AC_INIT([zebra], [0.1], [spadix@users.sourceforge.net])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_SRCDIR(zebra/scanner.c)

AC_SUBST([LIB_VERSION], [1:0:1])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_PROG_LIBTOOL
AC_PROG_RANLIB

AC_ARG_WITH([imagemagick],
  [AS_HELP_STRING([--without-imagemagick],
    [disable support for scanning images using ImageMagick])],
  [],
  [with_imagemagick=check])

AC_CHECK_TOOL([MAGICK_CONFIG], [Magick++-config])
AS_IF([test "x$with_imagemagick" != "xno" && \
       test "x$with_imagemagick" != "xcheck" && \
       test "x$MAGICK_CONFIG" = "x:"],
      [AC_MSG_FAILURE(
        [--with-imagemagick was given, but test for imagemagick failed])])
AM_CONDITIONAL([HAVE_MAGICK], [test "x$MAGICK_CONFIG" != "x:"])
AC_SUBST([MAGICK_CPPFLAGS], [`$MAGICK_CONFIG --cppflags`])
AC_SUBST([MAGICK_CXXFLAGS], [`$MAGICK_CONFIG --cxxflags`])
AC_SUBST([MAGICK_LDFLAGS], [`$MAGICK_CONFIG --ldflags`])
AC_SUBST([MAGICK_LIBS], [`$MAGICK_CONFIG --libs`])

AC_ARG_ENABLE([video],
  [AS_HELP_STRING([--disable-video],
    [exclude video scanner application])],
  [],
  [enable_video=check])

AC_CHECK_HEADER([linux/videodev.h],
  [ if test "x$enable_video" = "xcheck"; then enable_video="yes"; fi ],
  [ if test "x$enable_video" != "xno" && \
       test "x$enable_video" != "xcheck"; then
      AC_MSG_FAILURE(
        [--enable-video was given, but test for video support failed])
    fi
  ])

AC_ARG_WITH([sdl],
  [AS_HELP_STRING([--without-sdl],
    [disable support for live video display using SDL])],
  [],
  [ if test "x$enable_video" != "xno"; then
      with_sdl=check
    else
      with_sdl=no
    fi
  ])

AC_CHECK_TOOL([SDL_CONFIG], [sdl-config])
AS_IF([test "x$enable_video" = "xno"],
        [SDL_CONFIG=":"],
      [test "x$with_sdl" != "xno" && \
       test "x$with_sdl" != "xcheck" && \
       test "x$SDL_CONFIG" = "x:"],
        [AC_MSG_FAILURE(
          [--with-sdl was given, but test for SDL failed])])
AM_CONDITIONAL([HAVE_SDL], [test "x$SDL_CONFIG" != "x:"])
AC_SUBST([SDL_CPPFLAGS], [`$SDL_CONFIG --cflags`])
AC_SUBST([SDL_LIBS], [`$SDL_CONFIG --libs`])

AC_PATH_X

AC_HEADER_ASSERT
AC_CHECK_HEADERS([fcntl.h inttypes.h stdlib.h string.h sys/ioctl.h sys/time.h])

AC_TYPE_INT32_T
AC_TYPE_UINT32_T
AC_TYPE_UINT8_T

AC_C_CONST
AC_C_INLINE

AC_FUNC_MMAP
AC_CHECK_FUNCS([memset atexit setenv])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
