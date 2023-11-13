sbin_PROGRAMS = plpprintd
plpprintd_CPPFLAGS = -DPKGDATADIR="\"$(pkgdatadir)\"" -I$(top_srcdir)/lib -I$(top_srcdir)/intl
plpprintd_LDADD = $(LIB_PLP) $(INTLLIBS)
plpprintd_SOURCES = plpprintd.cc

EXTRA_DIST = prolog.ps.in fontmap

pkgdata_DATA = prolog.ps fontmap

maintainer-clean-local:
	rm -f prolog.ps
