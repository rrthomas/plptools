
libdir=$(kde_libraries)

INCLUDES = $(all_includes) -I$(top_srcdir)/lib
LDFLAGS  = $(all_libraries)

libplpprops_la_LDFLAGS = $(LIBDEBUG) $(all_libraries) -module -no-undefined -version-info $(LIBVERSION)

METASOURCES = AUTO

lib_LTLIBRARIES = libplpprops.la

libplpprops_la_SOURCES = plpprops.cc plppropsFactory.cc pie3dpiece.cpp \
	pie3dwidget.cpp
libplpprops_la_LIBADD = -L$(top_srcdir)/lib/.libs $(top_srcdir)/lib/libplp.la $(LIB_KIO) $(LIB_KFILE)
noinst_HEADERS = plpprops.h plppropsFactory.h pie3dpiece.h pie3dwidget.h

services_DATA = plpprops.desktop
servicesdir = $(kde_servicesdir)
EXTRA_DIST += $(services_DATA)

messages:
	$(XGETTEXT) -C -ki18n -kI18N_NOOP -ktranslate -kaliasLocale *.cc *.cpp *.h && mv messages.po ../po/libplpprops.pot
#
# remove all intermediate files that can be recreated using
# Makefile.cvs
#
maintainer-clean-local:
	rm -f Makefile.in *.closure
