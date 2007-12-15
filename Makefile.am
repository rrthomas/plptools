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
	@cvs tag rel-`echo $(VERSION) | sed -e 's/\./-/g'` && \
	read -e -p "SourceForge login: " USER && \
	read -r -s -p "Password: " PASSWORD && \
	echo -e "Release focus:\n\
  1 Initial FreshMeat announcement\n\
  2 Documentation\n\
  3 Code cleanup\n\
  4 Minor feature enhancements\n\
  5 Major feature enhancements\n\
  6 Minor bug fixes\n\
  7 Major bug fixes\n\
  8 Minor security fixes\n\
  9 Major security fixes" && \
	read -e -p "Choice: " FOCUS && \
	$$VISUAL $(NOTES) && \
	copher -u $$USER -P $$PASSWORD -p $(PACKAGE) -k $(PACKAGE) -r $(VERSION) -N $(NOTES) $(PACKAGE)-$(VERSION).tar.gz && \
	fm-submit --no-stdin -p $(PACKAGE) -v $(VERSION) -c "`cat $(NOTES)`" -r $$FOCUS
