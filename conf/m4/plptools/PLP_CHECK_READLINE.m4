dnl
dnl Check for libreadline and if it is available, check if it
dnl depends on libcurses. (Normally, it silently resolves the
dnl following symbols from libtermcap:
dnl   tgetnum, tgoto, tgetflag, tputs, tgetent, BC, PC, UP
dnl On RedHat 7.0 however, libtermcap is broken: It contains no
dnl symbols at all. Fortunately, libcurses provides the same
dnl and therefore we have to check for that special case.)
dnl
AC_DEFUN(PLP_CHECK_READLINE,
[
	AC_MSG_CHECKING(for readline in -lreadline)
	ac_cv_readline_libs=""
	rl_desc="Define this, if you have libreadline"
	saved_libs=$LIBS
	LIBS="$LIBS -lreadline"
	l1ok=0
	l2ok=0
	l3ok=0
	l4ok=0
	AC_TRY_LINK(,[extern char *readline(void); readline();],l1ok=1)
	LIBS="$saved_LIBS -lreadline -ltermcap"
	AC_TRY_LINK(,[extern char *readline(void); readline();],l2ok=1)
	LIBS="$saved_LIBS -lreadline -lcurses"
	AC_TRY_LINK(,[extern char *readline(void); readline();],l3ok=1)
	LIBS="$saved_LIBS -lreadline -lncurses"
	AC_TRY_LINK(,[extern char *readline(void); readline();],l4ok=1)
	LIBS="$saved_LIBS"
	case "$l1ok$l2ok$l3ok$l4ok" in
		0000)
			AC_MSG_RESULT(no)
			;;
		01*)
			dnl We prefer libtermcap cause it's smaller
			ac_cv_readline_libs="-lreadline -ltermcap"
			AC_MSG_RESULT([yes, and needs libtermcap])
			;;
		001*)
			dnl Prefer libcurses over libncurses
			ac_cv_readline_libs="-lreadline -lcurses"
			AC_MSG_RESULT([yes, and needs libcurses])
			;;
		0001)
			ac_cv_readline_libs="-lreadline -lncurses"
			AC_MSG_RESULT([yes, and needs libncurses])
			;;
		1*)
			ac_cv_readline_libs="-lreadline"
			AC_MSG_RESULT(yes)
			;;
	esac
	LIBREADLINE="${ac_cv_readline_libs}"
	AC_SUBST(LIBREADLINE)
	if test "${ac_cv_readline_libs}" != "" ; then
		AC_DEFINE_UNQUOTED(HAVE_LIBREADLINE,1,$rl_desc)
		PLP_READLINE_VERSION
	fi
])

dnl
dnl Check for readline version.
dnl Those readline developers change their API too frequently
dnl and don't provide a version number in the headers :-(
dnl
AC_DEFUN(PLP_READLINE_VERSION,
[
	AC_MSG_CHECKING(for readline version)
	saved_libs=$LIBS
	LIBS="$LIBS $LIBREADLINE"
	rl42=false
	AC_TRY_LINK(,[extern void rl_set_prompt(void); rl_set_prompt();],rl42=true)
	LIBS="$saved_LIBS"
	if $rl42 ; then
		AC_MSG_RESULT(4.2 or greater)
		AC_DEFINE_UNQUOTED(READLINE_VERSION,402)
	else
		AC_DEFINE_UNQUOTED(READLINE_VERSION,401)
		AC_MSG_RESULT(4.1 or less)
	fi
])
