SUBDIRS = api

EXTRA_DIST = ncpd.man.in plpnfsd.man.in plpftp.man.in

man_MANS = ncpd.8 plpnfsd.8 plpftp.1

%.1: %.man
	MANDATE=`grep CHECKIN $< | $(AWK) '{print $$4}'` ; \
	$(SED) -e "s%@MANDATE@%$$MANDATE%g" $< > $@

%.8: %.man
	MANDATE=`grep CHECKIN $< | $(AWK) '{print $$4}'` ; \
	$(SED) -e "s%@MANDATE@%$$MANDATE%g" $< > $@

