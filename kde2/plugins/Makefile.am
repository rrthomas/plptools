
AM_CPPFLAGS = -DQT_NO_CAST_ASCII

INCLUDES = $(all_includes) -I$(top_srcdir)/lib
LDFLAGS  = $(all_libraries) $(KDE_RPATH)

libplpprops_la_LDFLAGS = $(LIBDEBUG) $(all_libraries) -module -no-undefined -version-info $(LIBVERSION)

METASOURCES = AUTO

lib_LTLIBRARIES = libplpprops.la

libplpprops_la_SOURCES = plpprops.cc plppropsFactory.cc pie3dpiece.cpp \
	pie3dwidget.cpp
libplpprops_la_LIBADD = -L$(top_srcdir)/lib -lplp $(LIB_KIO) $(LIB_KFILE)
noinst_HEADERS = plpprops.h plppropsFactory.h pie3dpiece.h pie3dwidget.h

services_DATA = plpprops.desktop
servicesdir = $(kde_servicesdir)

#
# remove all intermediate files that can be recreated using
# Makefile.cvs
#
maintainer-clean-local:
	rm -f Makefile.in *.closure
