# $Id$
#
INCLUDES=-I$(top_srcdir)/lib

bin_PROGRAMS = sisinstall
sisinstall_LDADD = $(top_srcdir)/lib/libplp.la $(LIBNEWT)
sisinstall_SOURCES = psion.cpp sisinstaller.cpp sismain.cpp \
	fakepsion.cpp sisfilelink.cpp sisfilelink.h
EXTRA_DIST = psion.h sisinstaller.h fakepsion.h

maintainer-clean-local:
	rm -f Makefile.in
