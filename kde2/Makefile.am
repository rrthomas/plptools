# $Id$
#

SUBDIRS = kioslave plugins mime kpsion klipsi po doc

POFILES =
#
# remove all intermediate files that can be recreated using
# Makefile.cvs
#
maintainer-clean-local:
	rm -f Makefile.in

am_edit:
	perl ../conf/am_edit --topdir=kde2
