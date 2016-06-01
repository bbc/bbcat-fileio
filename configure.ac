AC_PREREQ([2.68])
AC_INIT([BBCAT_FILEIO],[0.1.2.2])

AS_NANO(BBCAT_FILEIO_CVS=no,BBCAT_FILEIO_CVS=yes)

AM_INIT_AUTOMAKE([1.11 silent-rules])
AM_SILENT_RULES([yes])

# Version number that will be appended to the library filename
BBCAT_FILEIO_MAJORMINOR=0.1
AC_SUBST(BBCAT_FILEIO_MAJORMINOR)

AC_CONFIG_HEADERS([config.h])

# CURRENT, REVISION, AGE
# - library source changed but interface the same -> increment REVISION
# - interfaces added/removed/changed -> increment CURRENT, set REVISION = 0
# - interfaces added -> increment AGE also
# - interfaces removed -> set AGE = 0
#    (AGE is the number of previous values of CURRENT that are compatible)
BBCAT_FILEIO_LIBVERSION="1:0:0"
AC_SUBST(BBCAT_FILEIO_LIBVERSION)

LT_PREREQ([2.2.6])
LT_INIT(disable-static win32-dll)

AC_PROG_CXX
AM_PROG_CC_C_O

AC_CONFIG_SRCDIR([src])

AC_CONFIG_MACRO_DIR([m4])
ACLOCAL_AMFLAGS="-I m4 $ACLOCAL_AMFLAGS"
AC_SUBST(ACLOCAL_AMFLAGS)

DX_DOXYGEN_FEATURE(ON)
DX_INIT_DOXYGEN(bbcat-fileio, doxygen.cfg)

dnl Set install paths.
dnl this is required because ${prefix} is used below
if test "x$prefix" = "xNONE" ; then
  prefix="/usr/local"
fi
if test "x$exec_prefix" = "xNONE" ; then
  exec_prefix=$prefix
fi

# Check if we should enable GPL code  (./configure --enable-gpl)
AC_MSG_CHECKING(whether to include code under gpl)
AC_ARG_ENABLE(gpl, AS_HELP_STRING([--disable-gpl], [disable code under GPL]), ENABLE_GPL="no", ENABLE_GPL="yes")
AM_CONDITIONAL(ENABLE_GPL, test "x${ENABLE_GPL}" = "xyes")
if test "x${ENABLE_GPL}" = "xyes"; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi

# Check if we should disable optimization  (./configure --disable-opt)
AC_MSG_CHECKING(whether to disable optimization)
AC_ARG_ENABLE(opt, AS_HELP_STRING([--disable-opt], [disable optimzation]), DISABLE_OPTIMIZATION="yes", DISABLE_OPTIMIZATION="no")
if test "x${DISABLE_OPTIMIZATION}" = "xyes"; then
  CFLAGS="$CFLAGS -O0"
  CXXFLAGS="$CXXFLAGS -O0"
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)

dnl bbcat-base is required
BBCAT_BASE_VER="0.1.2.0"
PKG_CHECK_MODULES(BBCAT_BASE, bbcat-base-0.1 >= $BBCAT_BASE_VER, HAVE_BBCAT_BASE=yes, HAVE_BBCAT_BASE=no)
if test "x${HAVE_BBCAT_BASE}" != xyes ; then
  AC_MSG_ERROR([bbcat-base-0.1 >= $BBCAT_BASE_VER is required])
fi

dnl bbcat-dsp is required
BBCAT_DSP_VER="0.1.2.0"
PKG_CHECK_MODULES(BBCAT_DSP, bbcat-dsp-0.1 >= $BBCAT_DSP_VER, HAVE_BBCAT_DSP=yes, HAVE_BBCAT_DSP=no)
if test "x${HAVE_BBCAT_DSP}" != xyes ; then
  AC_MSG_ERROR([bbcat-dsp-0.1 >= $BBCAT_DSP_VER is required])
fi

dnl bbcat-adm is required
BBCAT_ADM_VER="0.1.2.0"
PKG_CHECK_MODULES(BBCAT_ADM, bbcat-adm-0.1 >= $BBCAT_ADM_VER, HAVE_BBCAT_ADM=yes, HAVE_BBCAT_ADM=no)
if test "x${HAVE_BBCAT_ADM}" != xyes ; then
  AC_MSG_ERROR([bbcat-adm-0.1 >= $BBCAT_ADM_VER is required])
fi

TINYXML_VER="2.6.2"
PKG_CHECK_MODULES(TINYXML, tinyxml >= $TINYXML_VER, HAVE_TINYXML=yes, HAVE_TINYXML=no)
if test "x${HAVE_TINYXML}" != xyes ; then
  AC_MSG_ERROR([tinyxml >= $TINYXML_VER is required])
fi

BBCAT_GLOBAL_FILEIO_CFLAGS="$BBCAT_GLOBAL_FILEIO_CFLAGS $TINYXML_CFLAGS"
BBCAT_GLOBAL_FILEIO_LIBS="$BBCAT_GLOBAL_FILEIO_LIBS $TINYXML_LIBS"

# Check for GPL support
# only need ENABLE_GPL for inclusion of extra files, C flags are handled automatically!
EXISTING_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS $BBCAT_BASE_CFLAGS $BBCAT_DSP_CFLAGS $BBCAT_CONTROL_CFLAGS $BBCAT_RENDER_CFLAGS"

# Check for ENABLE_GPL
AC_MSG_CHECKING(whether GPL code is supported)
AC_TRY_COMPILE([],
                [#if !ENABLE_GPL
                 #error GPL not enabled
                 #endif
                ],
                [ENABLE_GPL="yes"],
                [ENABLE_GPL="no"])
if test "x${ENABLE_GPL}" = "xyes"; then
  AC_MSG_RESULT(yes)
else
  AC_MSG_RESULT(no)
fi
AM_CONDITIONAL(ENABLE_GPL, test "x${ENABLE_GPL}" = "xyes")
CFLAGS="$EXISTING_CFLAGS"

AC_SUBST(BBCAT_GLOBAL_FILEIO_CFLAGS)
AC_SUBST(BBCAT_GLOBAL_FILEIO_LIBS)

AC_APPEND_SUPPORTED_CFLAGS(BBCAT_FILEIO_CFLAGS, [-Wall])
AC_APPEND_SUPPORTED_CFLAGS(BBCAT_FILEIO_CFLAGS, [-Werror])
AC_APPEND_SUPPORTED_CFLAGS(BBCAT_FILEIO_CFLAGS, [-Wextra])
AC_APPEND_SUPPORTED_CFLAGS(BBCAT_FILEIO_CFLAGS, [-msse2])
AC_APPEND_SUPPORTED_CFLAGS(BBCAT_FILEIO_CFLAGS, [-fsigned-char])

AC_LANG_PUSH([C++])
AC_APPEND_SUPPORTED_CFLAGS(BBCAT_FILEIO_CXXFLAGS, [-std=c++11])
AC_LANG_POP([C++])

AC_SUBST(BBCAT_FILEIO_CFLAGS)

AC_SUBST([AM_CXXFLAGS],["$AM_CXXFLAGS $BBCAT_FILEIO_CXXFLAGS"])

BBCAT_FILEIO_LDFLAGS=""
AC_APPEND_SUPPORTED_LDFLAGS(BBCAT_FILEIO_LDFLAGS, [-Wl,--no-undefined])
AC_SUBST(BBCAT_FILEIO_LDFLAGS)

BBCAT_FILEIO_LIBS="\$(top_builddir)/src/libbbcat-fileio-$BBCAT_FILEIO_MAJORMINOR.la"
AC_SUBST(BBCAT_FILEIO_LIBS)

AC_SUBST(BBCAT_FILEIO_PKG_DEPS)

pkgconfigdir="\$(libdir)/pkgconfig"
AC_SUBST(pkgconfigdir)

libbbcat_fileiobindir="\$(libdir)/bbcat-fileio/"
AC_SUBST(libbbcat_fileiobindir)

AC_CONFIG_FILES([
Makefile
bbcat-fileio-uninstalled.pc
bbcat-fileio.pc
src/Makefile
test/Makefile
])
AC_OUTPUT
