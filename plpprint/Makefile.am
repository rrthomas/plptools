# $Id$
#
INCLUDES=-I$(top_srcdir)/lib

sbin_PROGRAMS = plpprintd
plpprintd_LDADD = $(top_srcdir)/lib/libplp.la
plpprintd_SOURCES = plpprintd.cc

pkgdata_DATA = prolog.ps fontmap

maintainer-clean-local:
	rm -f Makefile.in prolog.ps
