AC_DEFUN(PLP_CHECK_INTL,[
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
])

