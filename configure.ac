# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([libcomo], [0.2], [tero.isannainen@gmail.com])
AC_CONFIG_SRCDIR([src/como.c])
AC_CONFIG_HEADERS([src/config.h])
AM_INIT_AUTOMAKE
LT_INIT
AC_ENABLE_SHARED
AC_ENABLE_STATIC

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([plinth], [pl_alloc_memory], [], [])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h string.h unistd.h assert.h plinth.h])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset])

AC_OUTPUT(Makefile src/Makefile man/Makefile)
