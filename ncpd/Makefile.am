# $Id$
#
INCLUDES=-I../lib

HERE=$(shell pwd)
sbin_PROGRAMS = ncpd

ncpd_LDADD = ../lib/libplp.la
#ncpd_LDADD = -L$(HERE)/../lib -L$(HERE)/../lib/.libs -lplp
ncpd_SOURCES = channel.cc link.cc linkchan.cc main.cc \
	ncp.cc packet.cc socketchan.cc mp_serial.c
EXTRA_DIST = channel.h link.h linkchan.h mp_serial.h ncp.h packet.h socketchan.h
