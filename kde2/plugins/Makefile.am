
INCLUDES = $(all_includes) -I$(top_srcdir)/lib

kdemoduledir = $(kde_moduledir)
kdemodule_LTLIBRARIES = libplpprops.la

METASOURCES = AUTO

libplpprops_la_SOURCES = plpprops.cc plppropsFactory.cc pie3dpiece.cpp \
	pie3dwidget.cpp
libplpprops_la_LIBADD = $(LIB_PLP) $(LIB_KIO) $(LIB_KFILE)
libplpprops_la_LDFLAGS = $(PLP_LDFLAGS) $(KDE_LDFLAGS) $(LIBDEBUG) $(KDE_PLUGIN)

noinst_HEADERS = plpprops.h plppropsFactory.h pie3dpiece.h pie3dwidget.h

services_DATA = plpprops.desktop
servicesdir = $(kde_servicesdir)
EXTRA_DIST = $(services_DATA)

messages:
	$(XGETTEXT) -C -ki18n -kI18N_NOOP -ktranslate -kaliasLocale *.cc *.cpp *.h && mv messages.po ../po/libplpprops.pot
#
# remove all intermediate files that can be recreated using
# Makefile.cvs
#
maintainer-clean-local:
	rm -f Makefile.in *.closure
