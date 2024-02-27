# ncpd/Makefile.am
#
# This file is part of plptools.
#
# Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
# Copyright (C) 2007-2024 Reuben Thomas <rrt@sc3d.org>
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

AM_CPPFLAGS=-I$(top_srcdir)/lib
AM_CFLAGS = $(THREADED_CFLAGS)
AM_CXXFLAGS = $(THREADED_CXXFLAGS)

sbin_PROGRAMS = ncpd

ncpd_LDADD = $(LIB_PLP) $(INTLLIBS) $(LIBPMULTITHREAD) $(LIBTHREAD) $(NANOSLEEP_LIB) $(PTHREAD_SIGMASK_LIB) $(SELECT_LIB)

ncpd_SOURCES = channel.cc link.cc linkchan.cc main.cc \
	ncp.cc packet.cc socketchan.cc mp_serial.c
EXTRA_DIST = channel.h link.h linkchan.h main.h mp_serial.h ncp.h packet.h \
	socketchan.h
