# $Id$

maintainer-clean-local:
	rm -f Makefile.in *.closure

INCLUDES = $(all_includes) -I$(top_srcdir)/lib

kdemoduledir = $(kde_moduledir)
kdemodule_LTLIBRARIES = kio_plp.la

kio_plp_la_SOURCES = kio_plp.cpp
kio_plp_la_LIBADD = $(LIB_PLP) $(LIB_KIO)
kio_plp_la_LDFLAGS = $(PLP_LDFLAGS) $(KDE_LDFLAGS) $(LIBDEBUG) -module $(KDE_PLUGIN)

noinst_HEADERS = kio_plp.h

services_DATA = psion.protocol
servicesdir = $(kde_servicesdir)

EXTRA_DIST = $(services_DATA)
