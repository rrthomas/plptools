INCLUDES=-I$(top_srcdir)/lib

sbin_PROGRAMS = plpprintd
plpprintd_CPPFLAGS = -DPKGDATADIR="\"$(pkgdatadir)\""
plpprintd_LDADD = $(LIB_PLP) $(INTLLIBS)
plpprintd_SOURCES = plpprintd.cc

EXTRA_DIST = prolog.ps.in fontmap

pkgdata_DATA = prolog.ps fontmap

maintainer-clean-local:
	rm -f prolog.ps
