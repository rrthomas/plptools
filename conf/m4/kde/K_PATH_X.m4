dnl ------------------------------------------------------------------------
dnl Find the header files and libraries for X-Windows. Extended the
dnl macro AC_PATH_X
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN(K_PATH_X,[
	AC_REQUIRE([AC_PROG_CPP])
	AC_MSG_CHECKING(for X)
	AC_LANG_SAVE
	AC_LANG_C
	AC_CACHE_VAL(ac_cv_have_x,[
		# One or both of the vars are not set, and there is no
		# cached value.
		if test "{$x_includes+set}" = set || test "$x_includes" = NONE; then
			kde_x_includes=NO
		else
			kde_x_includes=$x_includes
		fi
		if test "{$x_libraries+set}" = set || test "$x_libraries" = NONE; then
			kde_x_libraries=NO
		else
			kde_x_libraries=$x_libraries
		fi

		# below we use the standard autoconf calls
		ac_x_libraries=$kde_x_libraries
		ac_x_includes=$kde_x_includes

		AC_PATH_X_DIRECT
		AC_PATH_X_XMKMF
		if test -z "$ac_x_includes"; then
			ac_x_includes="."
		fi
		if test -z "$ac_x_libraries"; then
			ac_x_libraries="/usr/lib"
		fi
		#from now on we use our own again

		# when the user already gave --x-includes, we ignore
		# what the standard autoconf macros told us.
		if test "$kde_x_includes" = NO; then
			kde_x_includes=$ac_x_includes
		fi

		if test "$kde_x_includes" = NO; then
			AC_MSG_ERROR([Can't find X includes. Please check your installation and add the correct paths!])
		fi

		if test "$ac_x_libraries" = NO; then
			AC_MSG_ERROR([Can't find X libraries. Please check your installation and add the correct paths!])
		fi

		# Record where we found X for the cache.
		ac_cv_have_x="have_x=yes \
			kde_x_includes=$kde_x_includes ac_x_libraries=$ac_x_libraries"
	])dnl
	eval "$ac_cv_have_x"

	if test "$have_x" != yes; then
		AC_MSG_RESULT($have_x)
		no_x=yes
	else
		AC_MSG_RESULT([libraries $ac_x_libraries, headers $kde_x_includes])
	fi

	if test -z "$kde_x_includes" || test "x$kde_x_includes" = xNONE; then
		X_INCLUDES=""
		x_includes="."; dnl better than nothing :-
	else
		x_includes=$kde_x_includes
		X_INCLUDES="-I$x_includes"
	fi

	if test -z "$ac_x_libraries" || test "x$ac_x_libraries" = xNONE; then
		X_LDFLAGS=""
		x_libraries="/usr/lib"; dnl better than nothing :-
	else
		x_libraries=$ac_x_libraries
		X_LDFLAGS="-L$x_libraries"
	fi
	all_includes="$all_includes $X_INCLUDES"
	all_libraries="$all_libraries $X_LDFLAGS"

	AC_SUBST(X_INCLUDES)
	AC_SUBST(X_LDFLAGS)
	AC_SUBST(x_libraries)
	AC_SUBST(x_includes)

	# Check for libraries that X11R6 Xt/Xaw programs need.
	ac_save_LDFLAGS="$LDFLAGS"
	test -n "$x_libraries" && LDFLAGS="$LDFLAGS -L$x_libraries"

	# SM needs ICE to (dynamically) link under SunOS 4.x (so we have to
	# check for ICE first), but we must link in the order -lSM -lICE or
	# we get undefined symbols.  So assume we have SM if we have ICE.
	# These have to be linked with before -lX11, unlike the other
	# libraries we check for below, so use a different variable.
	#  --interran@uluru.Stanford.EDU, kb@cs.umb.edu.
	AC_CHECK_LIB(ICE, IceConnectionNumber,
		[LIBSM="$X_PRELIBS -lSM"], , $X_EXTRA_LIBS)
	AC_SUBST(LIBSM)
	LDFLAGS="$ac_save_LDFLAGS"

	AC_SUBST(X_PRE_LIBS)

	LIB_X11='-lX11 $(LIBSOCKET)'
	AC_SUBST(LIB_X11)

	AC_MSG_CHECKING(for libXext)
	AC_CACHE_VAL(kde_cv_have_libXext,[
		kde_ldflags_safe="$LDFLAGS"
		kde_libs_safe="$LIBS"

		LDFLAGS="$X_LDFLAGS $USER_LDFLAGS"
		LIBS="-lXext -lX11 $LIBSOCKET"

		AC_TRY_LINK([#include <stdio.h>],[printf("hello Xext\n");],
			kde_cv_have_libXext=yes,
			kde_cv_have_libXext=no
		)

		LDFLAGS=$kde_ldflags_safe
		LIBS=$kde_libs_safe
	])

	AC_MSG_RESULT($kde_cv_have_libXext)

	if test "kde_cv_have_libXext" = "no"; then
		AC_MSG_ERROR([We need a working libXext to proceed. Since configure
can't find it itself, we stop here assuming that make wouldn't find
them either.])
	fi

	AC_LANG_RESTORE
])
