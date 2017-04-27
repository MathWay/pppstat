#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT([PPPstat], [0.5.0], [mathway@narod.ru])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE(pppstat, 0.5.0)
#AC_CONFIG_HEADERS([config.h])
AM_CONFIG_HEADER(config.h)
AC_PROG_MAKE_SET

ALL_LINGUAS="uk ru"
# i18n macros
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.14.4])

# Checks for programs.
AC_PROG_CC
#AC_PROG_INSTALL

AC_PATH_PROG([BZIP], [bzip2], [no])
if test "$BZIP" = "no"; then
    echo "Error: bzip2 missing."; exit 1
fi

AC_PATH_PROG([BUNZIP], [bunzip2], [no])
if test "$BUNZIP" = "no"; then
    echo "Error: bunzip2 missing."; exit 1
fi

AC_PATH_PROG([GZIP], [gzip], [no])
if test "$GZIP" = "no"; then
    echo "Error: gzip missing."; exit 1
fi

AC_PATH_PROG([GUNZIP], [gunzip], [no])
if test "$GUNZIP" = "no"; then
    echo "Error: gunzip missing."; exit 1
fi

# Checks for files
#FIXME
#AC_CHECK_FILE([/var/log/messages],,
#AC_MSG_CHECKING([for var/log/messages])
#if test ! -e "/var/log/messages"; then
#    echo "Error: /var/log/messages missing."; exit 1
#fi
#AC_MSG_RESULT([yes])
    

# Checks for libraries.
AC_CHECK_LIB([m], [lrint])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([libintl.h locale.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([setlocale strchr strstr])


AC_CONFIG_FILES([
Makefile
src/Makefile 
po/Makefile.in
intl/Makefile
config/Makefile
cron/Makefile
src/conf.h])

AC_OUTPUT
