## Makefile.am of kdebase/kioslave/plp

INCLUDES = $(all_includes) -I$(top_srcdir)/lib
LDFLAGS  = $(all_libraries) $(KDE_RPATH)

####### Files

kio_plp_la_LDFLAGS = -module -avoid-version -no-undefined

if BUILD_KDE

myprotodir = $(kde_servicesdir)

lib_LTLIBRARIES = kio_plp.la

kio_plp_la_SOURCES = kio_plp.cpp
kio_plp_la_LIBADD = -L$(top_srcdir)/lib -lplp -lkio
noinst_HEADERS = kio_plp.h

myproto_DATA = psion.protocol

METASOURCES = AUTO

bin_SCRIPTS = 

endif
