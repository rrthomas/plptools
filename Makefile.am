ACLOCAL_AMFLAGS = -I m4

SUBDIRS = intl po lib ncpd plpfuse plpftp plpprint sisinstall doc

EXTRA_DIST = AUTHORS COPYING INSTALL NEWS README TODO HISTORY ABOUT-NLS \
	etc/s5magic etc/s5kdemagic etc/psion.in etc/psion.SuSE.in \
	etc/ttytap.c etc/psidump etc/psidump.1 \
	etc/udev-usbserial-plptools.rules etc/plptools.spec

DISTCLEANFILES = etc/psion etc/psion.SuSE

install-exec-local:
	install -D $(top_builddir)/etc/psion $(DESTDIR)$(initdir)/psion

dist-local:
	kdoc -d doc/api -n libplp --strip-h-path lib/*.h

NOTES = release-notes

release: distcheck
	cvs diff && \ # Stop if there are outstanding changes
	cvs tag rel-`echo $(VERSION) | sed -e 's/\./-/g'` && \
	woger $(PACKAGE) $(VERSION) release-notes.$(VERSION)
