dnl
dnl Check KDE version
dnl
AC_DEFUN(KDE_VERSION,
[
	ac_kde_version=2
	AC_MSG_CHECKING(for KDE version)
	tmp=`kde-config -v 2>/dev/null | grep KDE:`
	case "$tmp" in
		KDE:?3.*)
			ac_kde_version=3
			;;
	esac
	AC_MSG_RESULT($ac_kde_version.x)
	kdeversion=$ac_kde_version
	AC_SUBST(kdeversion)
])
