#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([watch-gettext], [0.3.1], [Stanislav Brabec <sbrabec@suse.cz>])
AC_CONFIG_SRCDIR([watch-gettext.c])
AC_CONFIG_MACRO_DIR([m4])
AM_DISABLE_STATIC
AM_INIT_AUTOMAKE([-Wall -Werror dist-bzip2 no-dist-gzip])
AM_MAINTAINER_MODE
AC_GNU_SOURCE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AM_PROG_AR
LT_INIT

# Checks for libraries.
AC_CHECK_LIB([dl], [dlsym])
AM_PATH_GLIB_2_0

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_SUBST([AM_CPPFLAGS], ["-Wall -Wextra"])
AC_SUBST([AM_CFLAGS], ["-Wall -Wextra"])

# Checks for header files.
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([dlwrap/dlwrap.h],,[
	AC_MSG_ERROR([You need dlwrap header files.])
])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
