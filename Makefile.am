# $Id$
#
SUBDIRS = lib ncpd plpnfsd plpftp doc
EXTRA_DIST = COPYING README TODO INSTALL README.mjg include/*.h* etc/*magic etc/*.in patches
DISTCLEANFILES = etc/psion
AUTOMAKE_OPTIONS = foreign

#
# remove all intermediate files that can be recreated using
# Makefile.cvs
#
maintainer-clean-local:
	rm -f configure aclocal.m4 config.guess config.sub Makefile.in \
		install-sh ltconfig ltmain.sh missing mkinstalldirs

kdoc:
	kdoc -d doc/api -n libplp lib/*.h
