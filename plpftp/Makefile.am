# $Id$
#
INCLUDES=-I$(top_srcdir)/lib

#if HAVE_LIBREADLINE
LIBREADLINE=-lreadline
#endif
#if HAVE_LIBHISTORY
LIBHISTORY=-lhistory
#endif

bin_PROGRAMS = plpftp
plpftp_LDADD = $(top_srcdir)/lib/libplp.la $(LIBREADLINE) $(LIBHISTORY)
plpftp_SOURCES = ftp.cc main.cc
EXTRA_DIST = ftp.h

maintainer-clean-local:
	rm -f Makefile.in
