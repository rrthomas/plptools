libdir=$(kde_libraries)
 
bindir=$(kde_bindir)
  
maintainer-clean-local:
	rm -f Makefile.in
	   
METASOURCES = AUTO
	    
KDE_ICON = AUTO
	     
INCLUDES = $(all_includes) -I$(top_srcdir)/lib
	      
DISTCLEANFILES = $(klipsi_METASOURCES) ./.deps/* ./.deps/.P

bin_PROGRAMS = klipsi
lib_LTLIBRARIES = klipsi.la

CLEANFILES = dummy.cpp

klipsi_la_LIBADD = -L$(top_srcdir)/lib/.libs $(top_srcdir)/lib/libplp.la $(LIB_KDEUI)
klipsi_la_SOURCES = main.cpp toplevel.cpp
klipsi_la_LDFLAGS = $(all_libraries) -module -avoid-version

klipsi_LDADD = klipsi.la
klipsi_SOURCES = dummy.cpp
klipsi_LDFLAGS = $(all_libraries) $(KDE_RPATH)


applnkdir = $(kde_appsdir)/Utilities
applnk_DATA = klipsi.desktop
EXTRA_DIST += $(applnk_DATA)

appsdir = $(kde_datadir)/klipsi
apps_DATA = eventsrc
EXTRA_DIST += $(apps_DATA)

noinst_HEADERS = toplevel.h
	 
messages:
	$(XGETTEXT) -C -ki18n -kI18N_NOOP -ktranslate -kaliasLocale *.cpp *.h && mv messages.po ../po/klipsi.pot

dummy.cpp:
	: > dummy.cpp
