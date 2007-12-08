AC_REVISION($Revision$)dnl revision of this configure.in script

AC_INIT(Makefile.am)
AC_GNU_SOURCE
AC_CANONICAL_SYSTEM
AM_CONFIG_HEADER(include/config.h)
AM_INIT_AUTOMAKE(plptools, 0.19)
AM_PROG_LIBTOOL

dnl Set library version
maj=$(echo ${VERSION} | cut -d. -f1)
min=$(echo ${VERSION} | cut -d. -f2)
LIBVERSION=${maj}:${min}:${maj}
AC_SUBST(LIBVERSION)

dnl Enable Maintainer stuff
AM_MAINTAINER_MODE

dnl checks for programs
AM_PROG_CC_C_O
AC_PROG_MAKE_SET
AC_PROG_AWK
AC_PATH_PROG(SED,sed)

dnl Debugging
AC_MSG_CHECKING([if the debugging code should be included])
AC_ARG_ENABLE(debug,
AC_HELP_STRING([--enable-debug], [include debugging code]),
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

dnl NLS support
ALL_LINGUAS="de sv"
AM_GNU_GETTEXT
dnl Additional glibc-intl checks
if test "${USE_NLS}" = "yes" ; then
	if test "${USE_INCLUDED_LIBINTL}" = "yes" ; then
		plp_have_bind_textdomain_codeset=no
	else
		AC_CHECK_LIB(c, bind_textdomain_codeset,
			plp_have_bind_textdomain_codeset=yes,
			plp_have_bind_textdomain_codeset=no)
	fi
	if test "${plp_have_bind_textdomain_codeset}" = yes ; then
		AC_DEFINE(HAVE_BIND_TEXTDOMAIN_CODESET)
	fi
fi

dnl Large File Support
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

dnl checking for aix
AC_MSG_CHECKING(for AIX)
if test "`uname -s 2>/dev/null`" = AIX ; then
  AC_MSG_RESULT(yes)
  AC_CHECK_LIB(bsd, hostname)
  EXTRA_OBJS=mount_aix.o
  AC_DEFINE([_ALL_SOURCE], [], [Define _ALL_SOURCE on AIX])
else
  AC_MSG_RESULT(no)
fi
AC_SUBST(EXTRA_OBJS)

dnl checks for libraries

dnl readline and history for plpftp
ac_enable_readline=yes
AC_ARG_ENABLE(readline,
	[  --disable-readline      disable libreadline support [no]],
	if test "$enableval" = "no" ; then
		AC_MSG_RESULT([support for libreadline disabled])
		ac_enable_readline=no
	fi
)
if test "x$ac_enable_readline" = "xyes" ; then
	AC_MSG_CHECKING(for readline in -lreadline)
        LIBREADLINE=
        AC_CHECK_LIB([readline], [main],
              [AC_SUBST([LIBREADLINE], ["-lreadline -lncurses"])
               AC_DEFINE([HAVE_LIBREADLINE], [1],
                         [Define if you have libreadline])
              ],
              , -lncurses)
fi

ac_enable_history=yes
ac_cv_libhistory=
AC_ARG_ENABLE(history,
	[  --disable-history       disable libhistory support [no]],
	if test "$enableval" = "no" ; then
		AC_MSG_RESULT([support for libhistory disabled])
		ac_enable_history=no
	fi
)
if test "x$ac_enable_history" = "xyes" ; then
	AC_CHECK_LIB(history, add_history,
	[
		AC_DEFINE_UNQUOTED(HAVE_LIBHISTORY,1,
			[Define this, if you have libhistory])
		ac_cv_libhistory=-lhistory
	])
fi
LIBHISTORY=${ac_cv_libhistory}
AC_SUBST(LIBHISTORY)

ac_enable_newt=yes
ac_cv_libnewt=
AC_ARG_ENABLE(newt,
	[  --disable-newt          disable libnewt support [no]],
	if test "$enableval" = "no" ; then
		AC_MSG_RESULT([support for libnewt disabled])
		ac_enable_newt=no
	fi
)
if test "x$ac_enable_newt" = "xyes" ; then
	AC_CHECK_LIB(newt, newtInit,
	[
		AC_DEFINE_UNQUOTED(HAVE_LIBNEWT,1,
			[Define this, if you have libnewt])
		ac_cv_libnewt=-lnewt
	])
fi
LIBNEWT=${ac_cv_libnewt}
AC_SUBST(LIBNEWT)

dnl these three are for solaris
AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(nsl, gethostbyname)
ac_save_LDFLAGS=$LDFLAGS; LDFLAGS="-L/usr/ucblib -R/usr/ucblib $LDFLAGS"
AC_CHECK_LIB(rpcsoc, svcudp_create,,LDFLAGS=$ac_save_LDFLAGS)

dnl checks for mount table - if desired
ac_enable_mnttab=yes
AC_ARG_ENABLE(mnttab,
        [  --disable-mnttab        disable mnttab writing [no]],
        if test "$enableval" = "no" ; then
                AC_MSG_RESULT([support for mnttab disabled])
                ac_enable_mnttab=no
        fi
)
if test "x$ac_enable_mnttab" = "xyes" ; then
        AC_MSG_CHECKING(for mount table)
       	MTAB=NO
	for i in /etc/mnttab /etc/mtab; do
		if test -r "$i" ; then
			MTAB=$i
			break 2
		fi
	done
        AC_MSG_RESULT($MTAB)
        if test "$MTAB" = "NO" ; then
                AC_DEFINE_UNQUOTED(DONT_UPDATE_MTAB,1,
	                [Define this this if you want to prevent plpnfsd from updating mtab])
        else
                AC_DEFINE_UNQUOTED(MTAB_PATH,"$MTAB",
			[Define this to your mtab's path])
	        ac_cv_mtab_tmp="${MTAB}~"
                case "$target_alias" in
                        *linux*)
                                ;;
                        *)
                                ac_cv_mtab_tmp="${MTAB}.plpnfsd"
                                ;;
                esac
                AC_DEFINE_UNQUOTED(MTAB_TMP,"${ac_cv_mtab_tmp}",
			[Define this to your temporary mtab's path])
	fi
fi

dnl Check, if bot time.h and sys/time.h may be included
AC_HEADER_TIME

dnl Type of sighandlers
AC_TYPE_SIGNAL

dnl checks for header files
AC_CHECK_HEADERS(
	sys/time.h sys/ioctl.h sys/errno.h sys/ttold.h stdlib.h \
	sys/int_types.h stdint.h
)

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
    [  --with-drive=DRIVE      override default Psion drive [AUTO]],
    [ DDRIVE="$withval"
      AC_MSG_RESULT(Overriding drive: $DDRIVE) ],
    [ DDRIVE='AUTO'
      AC_MSG_RESULT(Using default Psion drive: $DDRIVE)
    ]
)
AC_DEFINE_UNQUOTED(DDRIVE,"$DDRIVE",
	[Define this to your default drive on your Psion])

AC_ARG_WITH(basedir,
    [  --with-basedir=DIR      override default Psion directory [\\\\]],
    [ DBASEDIR="$withval"
      AC_MSG_RESULT(Overriding directory: $DBASEDIR) ],
    [ DBASEDIR='\\'
      AC_MSG_RESULT(Using default Psion directory: $DBASEDIR)
    ]
)
AC_DEFINE_UNQUOTED(DBASEDIR,"$DBASEDIR",
	[Define this to your default directory on your Psion])

AC_ARG_WITH(mountdir,
    [  --with-mountdir=DIR     override default mount point [/mnt/psion]],
    [ DMOUNTPOINT="$withval"
      AC_MSG_RESULT(Overriding mount point: $DMOUNTPOINT) ],
    [ DMOUNTPOINT="/mnt/psion"
      AC_MSG_RESULT(Using default mount point: $DMOUNTPOINT)
    ]
)
AC_DEFINE_UNQUOTED(DMOUNTPOINT,"$DMOUNTPOINT",
	[Define this to your default mountpoint for plpnfsd])
AC_SUBST(DMOUNTPOINT)
test "x$prefix" = xNONE && prefix="$ac_default_prefix"
eval PKGDATA="${datadir}/${PACKAGE}"
AC_DEFINE_UNQUOTED(PKGDATA,"$PKGDATA",[This defines the dir for support files])
AC_SUBST(PKGDATA)

AC_ARG_WITH(initdir,
    [  --with-initdir=DIR      override default init dir [/etc/rc.d/init.d]],
    [ initdir="$withval"
      AC_MSG_RESULT(Overriding initdir: $initdir) ],
    [ initdir="/etc/rc.d/init.d"
      AC_MSG_RESULT(Using default init dir: $initdir)
    ]
)
AC_SUBST(initdir)

LIB_PLP='$(top_builddir)/lib/libplp.la'
AC_SUBST(LIB_PLP)
PLP_LDFLAGS='-L$(top_builddir)/lib/.libs'
AC_SUBST(PLP_LDFLAGS)

dnl Extra definitions for config.h
AH_BOTTOM([
/* Define this, if libc provides bind_textdomain_codeset */
#undef HAVE_BIND_TEXTDOMAIN_CODESET

/* Some reasonable defaults */
#ifndef PSIONHOSTNAME
# define PSIONHOSTNAME "localhost"
#endif

/* misc tweaks */
#ifdef _IBMR2
# undef DONT_UPDATE_MTAB
# define DONT_UPDATE_MTAB /* The mount table is obtained from the kernel (!?) */
#endif
])


AC_OUTPUT(
	Makefile
	intl/Makefile
	po/Makefile.in
	lib/Makefile
	ncpd/Makefile
	plpftp/Makefile
	plpnfsd/Makefile
	plpprint/Makefile
	plpprint/prolog.ps
	sisinstall/Makefile
	doc/Makefile
	doc/api/Makefile
	etc/psion
	etc/psion.SuSE
	etc/plptools.spec
	doc/ncpd.man
	doc/plpnfsd.man
	doc/plpftp.man
	doc/sisinstall.man
	doc/plpprintd.man
)
