# $Id$

maintainer-clean-local:
	rm -f Makefile.in

INCLUDES = $(all_includes) -I$(top_srcdir)/lib
LDFLAGS  = $(all_libraries) $(KDE_RPATH)

kdemoduledir = $(kde_moduledir)
kdemodule_LTLIBRARIES = kio_plp.la

kio_plp_la_LDFLAGS = -module $(KDE_PLUGIN)

kio_plp_la_SOURCES = kio_plp.cpp
kio_plp_la_LIBADD = -L$(top_srcdir)/lib/.libs $(top_srcdir)/lib/libplp.la \
	$(LIB_KIO)

noinst_HEADERS = kio_plp.h

services_DATA = psion.protocol
servicesdir = $(kde_servicesdir)

EXTRA_DIST = $(services_DATA)
