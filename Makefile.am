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

uninstall-local:
	rm -f $(DESTDIR)$(initdir)/plptools

install-exec-local:
	install -D $(top_builddir)/etc/plptools $(DESTDIR)$(initdir)/plptools

release: distcheck
	git diff && \
	git tag rel-`echo $(VERSION) | sed -e 's/\./-/g'` && git push --tag \
	woger sourceforge package=$(PACKAGE) version=$(VERSION) notes=release-notes-$(VERSION) dist_type="tar.gz"
