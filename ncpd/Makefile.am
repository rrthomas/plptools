# $Id$
#
INCLUDES=-I../lib

sbin_PROGRAMS = ncpd

ncpd_LDADD = ../lib/libplp.la
ncpd_SOURCES = channel.cc link.cc linkchan.cc main.cc \
	ncp.cc packet.cc socketchan.cc mp_serial.c
EXTRA_DIST = channel.h link.h linkchan.h mp_serial.h ncp.h packet.h socketchan.h
