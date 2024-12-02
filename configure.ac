#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([skarabeusz], [0.0.2], [pawel.f.biernacki@gmail.com])
AM_INIT_AUTOMAKE()
AC_CONFIG_HEADERS([config.h])
AM_GNU_GETTEXT([external])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
PKG_PROG_PKG_CONFIG
PKG_INSTALLDIR

# Checks for libraries.
PKG_CHECK_MODULES([GD], [gdlib >= 2.2.5])


AC_CONFIG_FILES([Makefile po/Makefile.in
                 example/Makefile
                 image/Makefile
                 inc/Makefile
                 src/Makefile])
AC_OUTPUT
