# doc/Makefile.am
#
# This file is part of plptools.
#
# Copyright (C) 2000-2002 Fritz Elfert <felfert@to.com>
# Copyright (C) 2007-2014 Reuben Thomas <rrt@sc3d.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# along with this program; if not, see <https://www.gnu.org/licenses/>.

EXTRA_DIST = ncpd.man.in plpfuse.man.in plpftp.man.in sisinstall.man.in \
	plpprintd.man.in

man_MANS = ncpd.8 plpftp.1 sisinstall.1 plpprintd.8
if BUILD_PLPFUSE
man_MANS += plpfuse.8
endif

edit = sed \
	-e 's|@MANDATE@|'`git log --pretty=format:"%ad" --date=short -1 $<.in`'|g' \
	-e 's|@pkgdatadir[@]|$(pkgdatadir)|g'

%.1: %.man Makefile
	rm -f $@ $@.tmp
	$(edit) $< >$@.tmp
	mv $@.tmp $@

%.8: %.man Makefile
	rm -f $@ $@.tmp
	$(edit) $< >$@.tmp
	mv $@.tmp $@

distclean-local:
	rm -f *.1 *.8
