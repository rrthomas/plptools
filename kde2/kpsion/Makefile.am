# $Id$

maintainer-clean-local:
	rm -f Makefile.in

METASOURCES = AUTO

KDE_ICON = AUTO

INCLUDES = $(all_includes) -I$(top_srcdir)/lib

DISTCLEANFILES = $(kpsion_METASOURCES) ./.deps/* ./.deps/.P

lib_LTLIBRARIES = libkpsion.la
bin_PROGRAMS	= kpsion

libkpsion_la_LDFLAGS = $(LIBDEBUG) $(all_libraries) -module -version-info $(LIBVERSION)
libkpsion_la_SOURCES = kpsion.cpp setupdialog.cpp wizards.cpp \
	statusbarprogress.cpp kpsionconfig.cpp kpsionbackuplistview.cpp \
	kpsionrestoredialog.cpp

libkpsion_la_LIBADD = -L$(top_srcdir)/lib/.libs -lplp $(LIB_KFILE)

kpsion_SOURCES	= main.cpp
kpsion_LDFLAGS  = $(KDE_RPATH)
kpsion_LDADD	= libkpsion.la $(top_srcdir)/lib/libplp.la

applnk_DATA = kpsion.desktop
applnkdir = $(kde_appsdir)/Utilities
EXTRA_DIST += kpsion.desktop

rcdir = $(kde_datadir)/kpsion
rc_DATA = kpsionui.rc
EXTRA_DIST += kpsionui.rc

noinst_HEADERS = kpsion.h setupdialog.h wizards.h kpsionrestoredialog.h \
	kpsionconfig.h kpsionbackuplistview.h statusbarprogress.h

messages:
	$(XGETTEXT) -C -ki18n -kI18N_NOOP -ktranslate -kaliasLocale *.cpp *.h && mv messages.po ../po/kpsion.pot
