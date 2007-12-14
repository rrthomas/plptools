INCLUDES=-I$(top_srcdir)/lib
AM_CFLAGS = $(THREADED_CFLAGS)
AM_CXXFLAGS = $(THREADED_CXXFLAGS)

sbin_PROGRAMS = ncpd

ncpd_LDADD = $(LIB_PLP) -lpthread $(INTLLIBS)
ncpd_SOURCES = channel.cc link.cc linkchan.cc main.cc \
	ncp.cc packet.cc socketchan.cc mp_serial.c
EXTRA_DIST = channel.h link.h linkchan.h main.h mp_serial.h ncp.h packet.h \
	socketchan.h
