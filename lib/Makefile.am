# $Id$
#
INCLUDES += -I$(top_srcdir)/intl
CFLAGS = $(THREADED_CFLAGS)
CXXFLAGS = $(THREADED_CXXFLAGS)

lib_LTLIBRARIES = libplp.la

libplp_la_LDFLAGS = $(LIBDEBUG) -version-info $(LIBVERSION)

libplp_la_SOURCES = bufferarray.cc  bufferstore.cc iowatch.cc ppsocket.cc \
	rfsv16.cc rfsv32.cc rfsvfactory.cc log.cc rfsv.cc rpcs32.cc rpcs16.cc \
	rpcs.cc rpcsfactory.cc psitime.cc Enum.cc plpdirent.cc wprt.cc \
	rclip.cc siscomponentrecord.cpp  sisfile.cpp sisfileheader.cpp \
	sisfilerecord.cpp sislangrecord.cpp sisreqrecord.cpp sistypes.cpp \
	psibitmap.cpp
pkginclude_HEADERS = bufferarray.h bufferstore.h iowatch.h ppsocket.h \
	rfsv.h rfsv16.h rfsv32.h rfsvfactory.h log.h rpcs32.h rpcs16.h rpcs.h \
	rpcsfactory.h psitime.h Enum.h plpdirent.h wprt.h plpintl.h rclip.h \
	siscomponentrecord.h sisfile.h sisfileheader.h sisfilerecord.h \
	sislangrecord.h sisreqrecord.h sistypes.h psibitmap.h

maintainer-clean-local:
	rm -f Makefile.in
