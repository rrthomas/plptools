dnl
AC_DEFUN(AC_PATH_KDE,[
	AC_BASE_PATH_KDE
	AC_ARG_ENABLE(path-check,
		[  --disable-path-check    don't try to find out, where to install],
		[
			if test "$enableval" = "no"; then
				ac_use_path_checking="default"
			else
				ac_use_path_checking=""
			fi
		], [ac_use_path_checking="default"]
	)

	AC_CREATE_KFSSTND($ac_use_path_checking)

	AC_SUBST_KFSSTND
	KDE_CREATE_LIBS_ALIASES
])
