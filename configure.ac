AC_INIT(plptools, 1.0.13, plptools-developers@lists.sourceforge.net)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE

dnl Basic settings
AC_GNU_SOURCE
AM_CONFIG_HEADER(config.h)
AM_PROG_LIBTOOL
AC_CONFIG_MACRO_DIR([m4])

dnl checks for programs
AC_PROG_CXX
AC_PROG_CC_C99
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

dnl Turn on additional compiler warnings
if test "$GCC" = "yes"; then
  CFLAGS="$CFLAGS -Wall -W -Wmissing-prototypes -Wstrict-prototypes -pedantic"
fi

dnl NLS support
ALL_LINGUAS="de sv"
AM_GNU_GETTEXT
AM_GNU_GETTEXT_VERSION([0.19.6])

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

# FUSE for plpfuse
PKG_CHECK_MODULES([FUSE], [fuse >= 2.6], [enable_fuse=yes], [enable_fuse=no])

# check for libattr for plpfuse
AC_CHECK_HEADER(attr/attributes.h,, enable_fuse=no)
AC_CHECK_LIB(attr, attr_set, [true], enable_fuse=no)

AM_CONDITIONAL(BUILD_PLPFUSE, test x$enable_fuse = xyes)

dnl these three are for Solaris
AC_CHECK_LIB(socket, socket)
AC_CHECK_LIB(nsl, gethostbyname)
ac_save_LDFLAGS=$LDFLAGS; LDFLAGS="-L/usr/ucblib -R/usr/ucblib $LDFLAGS"
AC_CHECK_LIB(rpcsoc, svcudp_create,,LDFLAGS=$ac_save_LDFLAGS)

dnl Check whether both time.h and sys/time.h may be included
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
/* Define this, if libc provides bind_textdomain_codeset */
#undef HAVE_BIND_TEXTDOMAIN_CODESET

/* Select version of FUSE API */
#define FUSE_USE_VERSION 26
])


AC_CONFIG_FILES(
        Makefile
        intl/Makefile
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
