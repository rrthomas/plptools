SUBDIRS = api

EXTRA_DIST = ncpd.man.in plpnfsd.man.in plpftp.man.in sisinstall.man.in \
	plpprintd.man.in plpbackup.man.in

man_MANS = ncpd.8 plpnfsd.8 plpftp.1 sisinstall.1 plpprintd.8 plpbackup.1

%.1: %.man
	MANDATE=`grep CHECKIN $< | $(AWK) '{print $$4}'` ; \
	$(SED) -e "s%@MANDATE@%$$MANDATE%g" $< > $@

%.8: %.man
	MANDATE=`grep CHECKIN $< | $(AWK) '{print $$4}'` ; \
	$(SED) -e "s%@MANDATE@%$$MANDATE%g" $< > $@

#
# remove all intermediate files that can be recreated using
# Makefile.cvs
#
maintainer-clean-local:
	rm -f Makefile.in *.1 *.8
