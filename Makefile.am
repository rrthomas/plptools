ACLOCAL_AMFLAGS = -I m4

SUBDIRS = intl po lib ncpd plpftp plpprint sisinstall doc
if BUILD_PLPFUSE
SUBDIRS += plpfuse
endif

EXTRA_DIST = AUTHORS COPYING INSTALL NEWS README TODO HISTORY ABOUT-NLS \
	etc/s5magic etc/s5kdemagic etc/plptools.in \
	etc/ttytap.c etc/psidump etc/psidump.1 \
	etc/udev-usbserial-plptools.rules

DISTCLEANFILES = etc/plptools

install-exec-local:
	install -D $(top_builddir)/etc/plptools $(DESTDIR)$(initdir)/plptools

dist-local:
	kdoc -d doc/api -n libplp --strip-h-path lib/*.h

NOTES = release-notes

release: distcheck
	cvs diff && \
	cvs tag rel-`echo $(VERSION) | sed -e 's/\./-/g'` && \
	woger $(PACKAGE) $(VERSION) release-notes-$(VERSION)
