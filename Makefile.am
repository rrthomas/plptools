# $Id$
#

if BUILD_KDE
KDESUB = kde2
else
KDESUB =
endif

SUBDIRS = intl po lib ncpd plpnfsd plpftp plpbackup plpprint sisinstall \
	$(KDESUB) doc .
EXTRA_DIST = CHANGES COPYING README TODO INSTALL ABOUT-NLS include/*.h* \
	etc/*magic etc/*.in etc/*.c etc/psidump* patches conf/m4/kde/*.m4 \
	conf/m4/plptools/*.m4 conf/[a-l]* conf/missing conf/mkinstalldirs \
	debian
DISTCLEANFILES = etc/psion etc/psion.SuSE etc/psion.SuSE8
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

install-data-local:
	test "$(initdir)" != "none" && \
		install -m 755 etc/psion $(DESTDIR)$(initdir)/psion || :

dist-hook:
	$(MAKE) TMPDEST=$(distdir)/ kdoc specfile
	$(MAKE) -C $(distdir)/kde2 -f Makefile.am am_edit
	conf/cvs2cl -W 1800 -S --no-wrap --prune --gmt -f $(distdir)/ChangeLog

kdoc:
	kdoc -d $(TMPDEST)doc/api -n libplp --strip-h-path $(TMPDEST)lib/*.h

specfile:
	sed -e "s/_VERSION_/$(VERSION)/" < $(TMPDEST)etc/plptools.spec.in \
		> $(TMPDEST)etc/plptools.spec
