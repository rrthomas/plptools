dnl
dnl Select first existing char device from a list of given devices
dnl Usage:
dnl   PLP_FIND_CDEV(/dev/tty1 /dev/tty2 /dev/console, FOUND)
dnl
dnl On return, variable FOUND is set to one of the devices or to NO, if none
dnl was found.
dnl
AC_DEFUN([PLP_FIND_CDEV],[
	$2=NO
	for i in $1; do
		if test -c "$i" ; then
			$2=$i
			break 2
		fi
	done
])
