# $Id$
#
INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/intl

if HAVE_LIBREADLINE
if ADD_LIBCURSES
LIBREADLINE=-lreadline -lcurses
else
LIBREADLINE=-lreadline
endif
endif
if HAVE_LIBHISTORY
LIBHISTORY=-lhistory
endif

bin_PROGRAMS = plpftp
plpftp_LDADD = $(top_srcdir)/lib/libplp.la $(LIBREADLINE) $(LIBHISTORY) $(LIBCCMALLOC_CXX) $(INTLLIBS)
plpftp_SOURCES = ftp.cc main.cc rlcrap.c
EXTRA_DIST = ftp.h rlcrap.h

maintainer-clean-local:
	rm -f Makefile.in
