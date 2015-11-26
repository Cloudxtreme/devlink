MAINTAINERCLEANFILES = Makefile.in

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = -I${top_srcdir}/include

dl_CFLAGS= $(LIBMNL_CFLAGS) -I${top_srcdir}/include -D_GNU_SOURCE
dl_LDADD = $(LIBMNL_LIBS) $(top_builddir)/libmnlg/libmnlg.la

bin_PROGRAMS=dl
dl_SOURCES=dl.c
