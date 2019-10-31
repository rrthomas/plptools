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

# FIXME: Automatically revert .po files if they're actually clean
# Currently, rather than run `git diff --exit-code', we simply show the
# diff, and eyeball it to check there's nothing other than the datestamps on
# the .po files.
release: distcheck
	git diff && \
	git tag rel-`echo $(VERSION) | sed -e 's/\./-/g'` && git push --tag && \
	woger github package=$(PACKAGE) version=$(VERSION) github_user=rrthomas github_dist_type=source dist_type="tar.gz"
