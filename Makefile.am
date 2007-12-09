# $Id$
#

SUBDIRS = intl po lib ncpd plpnfsd plpfuse plpftp plpprint sisinstall doc .

EXTRA_DIST = CHANGES COPYING README TODO HISTORY INSTALL ABOUT-NLS include/*.h* \
	etc/*magic etc/*.in etc/*.c etc/psidump* etc/udev-usbserial-plptools.rules \
	conf/m4/plptools/*.m4 conf/[a-l]* conf/missing conf/mkinstalldirs \
	debian/*
DISTCLEANFILES = etc/psion etc/psion.SuSE

TMPDEST=

install-data-local:
	test "$(initdir)" != "none" && \
		install -m 755 etc/psion $(DESTDIR)$(initdir)/psion || :

dist-hook:
	$(MAKE) TMPDEST=$(distdir)/ kdoc specfile
	test -f CVS/Root && cvs2cl -W 1800 -S --no-wrap --prune \
		--gmt -f $(distdir)/ChangeLog || \
		cp -f ChangeLog $(distdir)/ChangeLog
	rm -rf $(distdir)/debian/CVS

kdoc:
	kdoc -d $(TMPDEST)doc/api -n libplp --strip-h-path $(TMPDEST)lib/*.h

specfile:
	sed -e "s/\@VERSION\@/$(VERSION)/" < $(TMPDEST)etc/plptools.spec.in \
		> $(TMPDEST)etc/plptools.spec

ACLOCAL_AMFLAGS = -I m4
