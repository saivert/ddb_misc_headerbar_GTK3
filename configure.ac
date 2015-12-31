dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([ddb_misc_headerbar_GTK3], [1.0], [saivert@saivert.com])
AC_CONFIG_SRCDIR([src/headerbarui.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([foreign -Wall subdir-objects dist-xz no-dist-gzip])
AM_SILENT_RULES([yes])

dnl Override $PACKAGE to make $pkglibdir use deadbeef search path
PACKAGE=deadbeef

AC_PROG_CC
AM_PROG_AR
AC_PROG_INSTALL
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG
AC_PATH_PROG([GLIB_COMPILE_RESOURCES], [glib-compile-resources])

AC_CHECK_HEADERS([libintl.h stdlib.h string.h])
AC_TYPE_UINT32_T
AC_CHECK_FUNCS([atexit])


LT_INIT

AC_CHECK_HEADERS([deadbeef/deadbeef.h],,
    [
     AC_MSG_ERROR([missing required header (see above)])
    ])

PKG_CHECK_MODULES([GTK_DEPS], [gtk+-3.0], , AC_MSG_ERROR([gtk+-3.0 is required for this package]))
PKG_CHECK_MODULES([GLIB_DEPS], [glib-2.0], , AC_MSG_ERROR([glibc-2 is required for this package]))
dnl PKG_CHECK_MODULES([GIO_DEPS], [gio-2.0], , AC_MSG_ERROR([gio-2 is required for this package]))
dnl PKG_CHECK_MODULES([GTHREAD_DEPS], [gthread-2.0], , AC_MSG_ERROR([gthread-2 is required for this package]))

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
