# $Id$
#
INCLUDES=-I$(top_srcdir)/lib
CFLAGS = $(THREADED_CFLAGS)
CXXFLAGS = $(THREADED_CXXFLAGS)

sbin_PROGRAMS = ncpd

ncpd_LDADD = $(top_srcdir)/lib/libplp.la -lpthread $(LIBCCMALLOC_CXX)
ncpd_SOURCES = channel.cc link.cc linkchan.cc main.cc \
	ncp.cc packet.cc socketchan.cc mp_serial.c
EXTRA_DIST = channel.h link.h linkchan.h mp_serial.h ncp.h packet.h socketchan.h

maintainer-clean-local:
	rm -f Makefile.in
