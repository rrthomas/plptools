# $Id$
#

if BUILD_KDE
KDESUB = kde2
else
KDESUB =
endif

SUBDIRS = intl po lib ncpd plpnfsd plpftp plpbackup plpprint sisinstall \
	$(KDESUB) doc .
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

# Suse doesn't have chkconfig, so we need to create the symlinks here.
install-data-local:
	if [ -f /etc/SuSE-release ] ; then \
		install -m 755 etc/psion.SuSE $(DESTDIR)/etc/rc.d/init.d/psion ;\
		ln -s ../psion /etc/rc.d/rc2.d/S45psion ;\
		ln -s ../psion /etc/rc.d/rc3.d/S45psion ;\
		ln -s ../psion /etc/rc.d/rc4.d/S45psion ;\
		ln -s ../psion /etc/rc.d/rc5.d/S45psion ;\
		ln -s ../psion /etc/rc.d/rc1.d/K70psion ;\
		ln -s ../psion /etc/rc.d/rc0.d/K70psion ;\
		ln -s ../psion /etc/rc.d/rc6.d/K70psion ;\
		grep -v START_PSION /etc/rc.config > /etc/rc.config.psion ;\
		( \
		echo '# Set START_PSION to yes to start Psion support' ;\
		echo 'START_PSION=yes' ;\
		) >> /etc/rc.config.psion ;\
		cp /etc/rc.config /etc/rc.config.old ;\
		mv /etc/rc.config.psion /etc/rc.config ;\
	else \
		install -m 755 etc/psion $(DESTDIR)/etc/rc.d/init.d/psion ;\
	fi

dist-hook:
	$(MAKE) TMPDEST=$(distdir)/ kdoc specfile
	$(MAKE) -C $(distdir)/kde2 -f Makefile.am am_edit

kdoc:
	kdoc -d $(TMPDEST)doc/api -n libplp --strip-h-path $(TMPDEST)lib/*.h

specfile:
	sed -e "s/_VERSION_/$(VERSION)/" < $(TMPDEST)etc/plptools.spec.in \
		> $(TMPDEST)etc/plptools.spec
