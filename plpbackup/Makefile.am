# $Id$
#
INCLUDES=-I$(top_srcdir)/lib

bin_PROGRAMS = plpbackup
plpbackup_LDADD = $(top_srcdir)/lib/libplp.la
plpbackup_SOURCES = plpbackup.cc

maintainer-clean-local:
	rm -f Makefile.in
