# $Id$
#

if BUILD_KDE
KDESUB = kde2
else
KDESUB =
endif

SUBDIRS = intl po lib ncpd plpnfsd plpftp plpbackup $(KDESUB) doc .
EXTRA_DIST = CHANGES COPYING README TODO INSTALL README.mjg ABOUT-NLS include/*.h* etc/*magic etc/*.in patches conf/m4/kde/*.m4 conf/m4/plptools/*.m4 conf/[a-l]* conf/missing conf/mkinstalldirs
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
	etc/*.spec ABOUT-NLS
	find . -name "*~" -exec rm -f {} \;
	find . -name "\.\#*" -exec rm -f {} \;
	sh conf/intlclean

dist-hook:
	$(MAKE) TMPDEST=$(distdir)/ kdoc specfile
	$(MAKE) -C $(distdir)/kde2 -f Makefile.am am_edit

kdoc:
	kdoc -d $(TMPDEST)doc/api -n libplp --strip-h-path $(TMPDEST)lib/*.h

specfile:
	sed -e "s/_VERSION_/$(VERSION)/" < $(TMPDEST)etc/plptools.spec.in \
		> $(TMPDEST)etc/plptools.spec
