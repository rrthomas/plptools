# configure.ac
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

AC_PREREQ([2.71])

AC_INIT([plptools],[1.0.17],[plptools-developers@lists.sourceforge.net])
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE

dnl Basic settings
AC_USE_SYSTEM_EXTENSIONS
AC_CONFIG_HEADERS([config.h])
LT_INIT
AC_CONFIG_MACRO_DIR([m4])

dnl checks for programs
AC_PROG_INSTALL
AC_PROG_CXX
AC_PROG_CC

dnl Debugging
AC_MSG_CHECKING([if the debugging code should be included])
AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],[include debugging code]),
[
if test $enableval = yes; then
        CFLAGS="-g"
        if test "$GCC" = "yes"; then
          CFLAGS="$CFLAGS -ggdb"
        fi
        AC_MSG_RESULT(yes)
        AC_DEFINE(DEBUG, 1, [Define this to include debugging code.])
else
        AC_MSG_RESULT(no)
fi
], [
AC_MSG_RESULT(no)
])

dnl Turn on additional compiler warnings
if test "$GCC" = "yes"; then
  CFLAGS="$CFLAGS -Wall -W -Wmissing-prototypes -Wstrict-prototypes -pedantic"
fi

dnl NLS support
ALL_LINGUAS="de sv"
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.6])

dnl Large File Support
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

dnl checks for libraries

dnl readline and history for plpftp
MIN_LIBREADLINE_VERSION=4.3
AX_LIB_READLINE([$MIN_LIBREADLINE_VERSION])

# FUSE for plpfuse
PKG_CHECK_MODULES([FUSE], [fuse >= 2.6], [enable_fuse=yes], [enable_fuse=no])

# check for libattr for plpfuse
AC_CHECK_HEADER(attr/attributes.h,, enable_fuse=no)
AC_CHECK_HEADERS(attr/xattr.h)
AC_CHECK_LIB(attr, attr_set, [true], enable_fuse=no)

AM_CONDITIONAL(BUILD_PLPFUSE, test x$enable_fuse = xyes)

dnl these three are for Solaris
AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(nsl, gethostbyname)
ac_save_LDFLAGS=$LDFLAGS; LDFLAGS="-L/usr/ucblib -R/usr/ucblib $LDFLAGS"
AC_CHECK_LIB(rpcsoc, svcudp_create,,LDFLAGS=$ac_save_LDFLAGS)

dnl checks for header files
AC_CHECK_HEADERS(sys/time.h sys/ioctl.h sys/errno.h sys/ttold.h)

dnl special options for customization

AC_ARG_WITH(serial,
    [  --with-serial=DEV       override default serial line],
    [ DDEV="$withval"
      AC_MSG_RESULT(Overriding default serial device: $DDEV)
    ],
    [ DDEV="/dev/ttyS0"
      AC_MSG_RESULT(Using default serial device: $DDEV)]
)
AC_DEFINE_UNQUOTED(DDEV,"$DDEV",[Define this to your serial device node])
AC_SUBST(DDEV)

AC_ARG_WITH(speed,
    [  --with-speed=SPEED      override default serial speed [auto]],
    [ if "$withval" = "auto" ; then
        DSNAME=auto
        DSPEED=-1
      else
        DSPEED="$withval"
        DSNAME="$withval"
      fi
      AC_MSG_RESULT(Overriding serial speed: $DSNAME) ],
    [ DSPEED=-1
      DSNAME=auto
      AC_MSG_RESULT(Using default serial speed: auto)
    ]
)
AC_DEFINE_UNQUOTED(DSPEED,$DSPEED,[Define this to your serial device speed])
AC_DEFINE_UNQUOTED(DSNAME,$DSNAME,
        [Define this to your serial device speed alias])
AC_SUBST(DSPEED)
AC_SUBST(DSNAME)

AC_ARG_WITH(port,
    [  --with-port=PORT        override default port [7501]],
    [ DPORT="$withval"
      AC_MSG_RESULT(Overriding port: $DPORT) ],
    [ DPORT=7501
      AC_MSG_RESULT(Using default port: $DPORT)
    ]
)
AC_DEFINE_UNQUOTED(DPORT,$DPORT,
        [Define this to the TCP port ncpd should listen on])
AC_SUBST(DPORT)

AC_ARG_WITH(drive,
    [  --with-drive=DRIVE      override default EPOC drive [AUTO]],
    [ DDRIVE="$withval"
      AC_MSG_RESULT(Overriding drive: $DDRIVE) ],
    [ DDRIVE='AUTO'
      AC_MSG_RESULT(Using default EPOC drive: $DDRIVE)
    ]
)
AC_DEFINE_UNQUOTED(DDRIVE,"$DDRIVE",
        [Define this to your default drive on your EPOC device])

AC_ARG_WITH(basedir,
    [  --with-basedir=DIR      override default EPOC directory [\\\\]],
    [ DBASEDIR="$withval"
      AC_MSG_RESULT(Overriding directory: $DBASEDIR) ],
    [ DBASEDIR='\\'
      AC_MSG_RESULT(Using default EPOC directory: $DBASEDIR)
    ]
)
AC_DEFINE_UNQUOTED(DBASEDIR,"$DBASEDIR",
        [Define this to your default directory on your EPOC device])

AC_ARG_WITH(initdir,
    [  --with-initdir=DIR      override default init dir [PREFIX/etc/rc.d/init.d]],
    [ initdir="$withval"
      AC_MSG_RESULT(Overriding initdir: $initdir) ],
    [ initdir="${sysconfdir}/rc.d/init.d"
      AC_MSG_RESULT(Using default init dir: $initdir)
    ]
)
AC_SUBST(initdir)

LIB_PLP='$(top_builddir)/lib/libplp.la'
AC_SUBST(LIB_PLP)

dnl Extra definitions for config.h
AH_BOTTOM([
/* Select version of FUSE API */
#define FUSE_USE_VERSION 26
])


AC_CONFIG_FILES(
        Makefile
        po/Makefile.in
        lib/Makefile
        ncpd/Makefile
        plpftp/Makefile
        plpfuse/Makefile
        plpprint/Makefile
        plpprint/prolog.ps
        sisinstall/Makefile
        doc/Makefile
        etc/plptools
        doc/ncpd.man
        doc/plpfuse.man
        doc/plpftp.man
        doc/sisinstall.man
        doc/plpprintd.man
)
AC_OUTPUT

dnl Warn if readline is absent or too old
if test "$ax_cv_lib_readline_version_ok" != "yes"; then
   AC_MSG_WARN([To use readline, version $MIN_LIBREADLINE_VERSION or later is required!])
fi
