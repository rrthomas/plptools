# $Id$
#
INCLUDES += -I$(top_srcdir)/intl

lib_LTLIBRARIES = libplp.la

libplp_la_LDFLAGS = $(LIBDEBUG) -version-info $(LIBVERSION)

libplp_la_SOURCES = bufferarray.cc  bufferstore.cc iowatch.cc ppsocket.cc \
	rfsv16.cc rfsv32.cc rfsvfactory.cc log.cc rfsv.cc rpcs32.cc rpcs16.cc \
	rpcs.cc rpcsfactory.cc psitime.cc Enum.cc plpdirent.cc wprt.cc rclip.cc
pkginclude_HEADERS = bufferarray.h bufferstore.h iowatch.h ppsocket.h \
	rfsv.h rfsv16.h rfsv32.h rfsvfactory.h log.h rpcs32.h rpcs16.h rpcs.h \
	rpcsfactory.h psitime.h Enum.h plpdirent.h wprt.h plpintl.h rclip.h

maintainer-clean-local:
	rm -f Makefile.in
