dnl
dnl Select first existing file from a list of given files
dnl Usage:
dnl   PLP_FIND_FILE(filea fileb filec, FOUND)
dnl
dnl On return, variable FOUND is set to one of fileX or to NO, if none
dnl was found.
dnl
AC_DEFUN([PLP_FIND_FILE],[
	$2=NO
	for i in $1; do
		if test -r "$i" ; then
			$2=$i
			break 2
		fi
	done
])
