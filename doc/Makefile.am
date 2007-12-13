SUBDIRS = api

EXTRA_DIST = ncpd.man.in plpfuse.man.in plpftp.man.in sisinstall.man.in \
	plpprintd.man.in

man_MANS = ncpd.8 plpfuse.8 plpftp.1 sisinstall.1 plpprintd.8

%.1: %.man
	MANDATE=`grep CHECKIN $< | $(AWK) '{print $$4}'` ; \
	$(SED) -e "s%@MANDATE@%$$MANDATE%g" $< > $@

%.8: %.man
	MANDATE=`grep CHECKIN $< | $(AWK) '{print $$4}'` ; \
	$(SED) -e "s%@MANDATE@%$$MANDATE%g" $< > $@

maintainer-clean-local:
	rm -f *.1 *.8
