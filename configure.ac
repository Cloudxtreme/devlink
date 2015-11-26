#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([devlink], [0.1], [jiri@mellanox.com])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES(yes)], [])
AM_PROG_AR

# Here are a set of rules to help you update your library version information:
# 1. Start with version information of ‘0:0:0’ for each libtool library.
# 2. Update the version information only immediately before a public release
#    of your software. More frequent updates are unnecessary, and only guarantee
#    that the current interface number gets larger faster.
# 3. If the library source code has changed at all since the last update,
#    then increment revision (‘c:r:a’ becomes ‘c:r+1:a’).
# 4. If any interfaces have been added, removed, or changed since the last
#    update, increment current, and set revision to 0.
# 5. If any interfaces have been added since the last public release, then
#    increment age.
# 6. If any interfaces have been removed or changed since the last public
#    release, then set age to 0.

AC_SUBST(LIBMNLG_CURRENT, 0)
AC_SUBST(LIBMNLG_REVISION, 0)
AC_SUBST(LIBMNLG_AGE, 0)

CFLAGS="$CFLAGS -Wall"

# Checks for programs.
AC_PROG_CC
LT_INIT

PKG_CHECK_MODULES([LIBMNL], [libmnl])

# Checks for header files.
AC_CHECK_HEADERS([stdint.h stdlib.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile
include/Makefile \
libmnlg/Makefile \
libmnlg/libmnlg.pc \
utils/Makefile \
man/Makefile])
AC_OUTPUT
