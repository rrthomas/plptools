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
	ac_cv_addcurses=false
	ac_cv_have_libreadline=false
	saved_libs=$LIBS
	LIBS="$LIBS -lreadline"
	link1ok=0
	link2ok=0
	AC_TRY_LINK(,[extern char *readline(void); readline();],link1ok=1)
	LIBS="$LIBS -lreadline -lcurses"
	AC_TRY_LINK(,[extern char *readline(void); readline();],link2ok=1)
	LIBS="$saved_LIBS"
	case "$link1ok$link2ok" in
		00)
			AC_MSG_RESULT(no)
			;;
		01)
			ac_cv_have_libreadline=true;
			ac_cv_addcurses=true
			AC_DEFINE_UNQUOTED(HAVE_LIBREADLINE)
			AC_MSG_RESULT([yes, and needs libcurses])
			;;
		1*)
			ac_cv_have_libreadline=true;
			AC_DEFINE_UNQUOTED(HAVE_LIBREADLINE)
			AC_MSG_RESULT(yes)
			;;
	esac
	AM_CONDITIONAL(HAVE_LIBREADLINE, test x$ac_cv_have_libreadline = xtrue)
	AM_CONDITIONAL(ADD_LIBCURSES, test x$ac_cv_addcurses = xtrue)
])
