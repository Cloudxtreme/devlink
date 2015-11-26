MAINTAINERCLEANFILES = Makefile.in

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -fvisibility=hidden -ffunction-sections -fdata-sections
AM_LDFLAGS = -Wl,--gc-sections -Wl,--as-needed

lib_LTLIBRARIES = libmnlg.la
libmnlg_la_SOURCES = libmnlg.c
libmnlg_la_CFLAGS= $(LIBMNL_CFLAGS) $(AM_CFLAGS) -I${top_srcdir}/include -D_GNU_SOURCE
libmnlg_la_LIBADD= $(LIBMNL_LIBS)
libmnlg_la_LDFLAGS = $(AM_LDFLAGS) -version-info @LIBMNLG_CURRENT@:@LIBMNLG_REVISION@:@LIBMNLG_AGE@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmnlg.pc
