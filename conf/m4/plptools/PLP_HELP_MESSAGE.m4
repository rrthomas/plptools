dnl
dnl Insert a custom line into the output of configure --help
dnl
AC_DEFUN([PLP_HELP_MSG],[
	AC_DIVERT_PUSH(NOTICE)dnl
	ac_help="$ac_help
[$1]"
	AC_DIVERT_POP()dnl
])
