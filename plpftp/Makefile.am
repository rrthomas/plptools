# $Id$
#
INCLUDES = -I$(top_srcdir)/lib -I$(top_srcdir)/intl

bin_PROGRAMS = plpftp
plpftp_LDADD = $(LIB_PLP) $(LIBREADLINE) $(LIBHISTORY) $(INTLLIBS)
plpftp_SOURCES = ftp.cc main.cc rlcrap.c
EXTRA_DIST = ftp.h rlcrap.h

maintainer-clean-local:
	rm -f Makefile.in
