# $Id$
#
SUBDIRS = lib ncpd plpnfsd plpftp doc
EXTRA_DIST = CHANGES COPYING README TODO INSTALL README.mjg include/*.h* etc/*magic etc/*.in patches
DISTCLEANFILES = etc/psion
AUTOMAKE_OPTIONS = foreign

TMPDEST=
#
# remove all intermediate files that can be recreated using
# Makefile.cvs
#
maintainer-clean-local:
	rm -f configure aclocal.m4 config.guess config.sub Makefile.in \
		install-sh ltconfig ltmain.sh missing mkinstalldirs etc/*.spec

dist-hook:
	$(MAKE) TMPDEST=$(distdir)/ kdoc specfile

kdoc:
	kdoc -d $(TMPDEST)doc/api -n libplp $(TMPDEST)lib/*.h

specfile:
	sed -e "s/_VERSION_/$(VERSION)/" < $(TMPDEST)etc/plptools.spec.in \
		> $(TMPDEST)etc/plptools.spec
