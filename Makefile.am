# $Id$
#

if BUILD_KDE
KDESUB = kde2
else
KDESUB =
endif

SUBDIRS = lib ncpd plpnfsd plpftp plpbackup $(KDESUB) doc
EXTRA_DIST = CHANGES COPYING README TODO INSTALL README.mjg include/*.h* etc/*magic etc/*.in patches
DISTCLEANFILES = etc/psion
AUTOMAKE_OPTIONS = foreign

TMPDEST=
#
# remove all intermediate files that can be recreated using
# Makefile.cvs
#
maintainer-clean-local:
	rm -f configure aclocal.m4 Makefile.in libtool \
	conf/config.guess conf/config.sub conf/install-sh conf/ltconfig \
	conf/ltmain.sh conf/missing conf/mkinstalldirs \
	etc/*.spec
	find . -name "*~" -exec rm -f {} \;
	find . -name "\.\#*" -exec rm -f {} \;

dist-hook:
	$(MAKE) TMPDEST=$(distdir)/ kdoc specfile

kdoc:
	kdoc -d $(TMPDEST)doc/api -n libplp $(TMPDEST)lib/*.h

specfile:
	sed -e "s/_VERSION_/$(VERSION)/" < $(TMPDEST)etc/plptools.spec.in \
		> $(TMPDEST)etc/plptools.spec
