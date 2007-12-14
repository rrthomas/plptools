ACLOCAL_AMFLAGS = -I m4

SUBDIRS = intl po lib ncpd plpfuse plpftp plpprint sisinstall doc

EXTRA_DIST = AUTHORS COPYING INSTALL NEWS README TODO HISTORY ABOUT-NLS \
	etc/*magic etc/*.in etc/*.c etc/psidump* etc/udev-usbserial-plptools.rules \
	etc/plptools.spec

DISTCLEANFILES = etc/psion etc/psion.SuSE

install-data-local:
	install -m 755 etc/psion $(DESTDIR)$(initdir)/psion

dist-local:
	kdoc -d doc/api -n libplp --strip-h-path lib/*.h
