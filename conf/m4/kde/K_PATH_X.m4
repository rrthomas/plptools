# KDE_PATH_X_DIRECT
dnl Internal subroutine of AC_PATH_X.
dnl Set ac_x_includes and/or ac_x_libraries.
AC_DEFUN(KDE_PATH_X_DIRECT,
[if test "$ac_x_includes" = NO; then
  # Guess where to find include files, by looking for this one X11 .h file.
  test -z "$x_direct_test_include" && x_direct_test_include=X11/Intrinsic.h

  # First, try using that file with no special directory specified.
AC_TRY_CPP([#include <$x_direct_test_include>],
[# We can compile using X headers with no special include directory.
ac_x_includes=],
[# Look for the header file in a standard set of common directories.
# Check X11 before X11Rn because it is often a symlink to the current release.
  for ac_dir in               \
    /usr/X11/include          \
    /usr/X11R6/include        \
    /usr/X11R5/include        \
    /usr/X11R4/include        \
                              \
    /usr/include/X11          \
    /usr/include/X11R6        \
    /usr/include/X11R5        \
    /usr/include/X11R4        \
                              \
    /usr/local/X11/include    \
    /usr/local/X11R6/include  \
    /usr/local/X11R5/include  \
    /usr/local/X11R4/include  \
                              \
    /usr/local/include/X11    \
    /usr/local/include/X11R6  \
    /usr/local/include/X11R5  \
    /usr/local/include/X11R4  \
                              \
    /usr/X386/include         \
    /usr/x386/include         \
    /usr/XFree86/include/X11  \
                              \
    /usr/include              \
    /usr/local/include        \
    /usr/unsupported/include  \
    /usr/athena/include       \
    /usr/local/x11r5/include  \
    /usr/lpp/Xamples/include  \
                              \
    /usr/openwin/include      \
    /usr/openwin/share/include \
    ; \
  do
    if test -r "$ac_dir/$x_direct_test_include"; then
      ac_x_includes=$ac_dir
      break
    fi
  done])
fi # $ac_x_includes = NO

if test "$ac_x_libraries" = NO; then
  # Check for the libraries.

  test -z "$x_direct_test_library" && x_direct_test_library=Xt
  test -z "$x_direct_test_function" && x_direct_test_function=XtMalloc

  # See if we find them without any special options.
  # Don't add to $LIBS permanently.
  ac_save_LIBS="$LIBS"
  LIBS="-l$x_direct_test_library $LIBS"
AC_TRY_LINK(, [${x_direct_test_function}()],
[LIBS="$ac_save_LIBS"
# We can link X programs with no special library path.
ac_x_libraries=],
[LIBS="$ac_save_LIBS"
# First see if replacing the include by lib works.
# Check X11 before X11Rn because it is often a symlink to the current release.
for ac_dir in `echo "$ac_x_includes" | sed s/include/lib/` \
    /usr/X11/lib          \
    /usr/X11R6/lib        \
    /usr/X11R5/lib        \
    /usr/X11R4/lib        \
                          \
    /usr/lib/X11          \
    /usr/lib/X11R6        \
    /usr/lib/X11R5        \
    /usr/lib/X11R4        \
                          \
    /usr/local/X11/lib    \
    /usr/local/X11R6/lib  \
    /usr/local/X11R5/lib  \
    /usr/local/X11R4/lib  \
                          \
    /usr/local/lib/X11    \
    /usr/local/lib/X11R6  \
    /usr/local/lib/X11R5  \
    /usr/local/lib/X11R4  \
                          \
    /usr/X386/lib         \
    /usr/x386/lib         \
    /usr/XFree86/lib/X11  \
                          \
    /usr/lib              \
    /usr/local/lib        \
    /usr/unsupported/lib  \
    /usr/athena/lib       \
    /usr/local/x11r5/lib  \
    /usr/lpp/Xamples/lib  \
    /lib/usr/lib/X11	  \
                          \
    /usr/openwin/lib      \
    /usr/openwin/share/lib \
    ; \
do
dnl Don't even attempt the hair of trying to link an X program!
  for ac_extension in a so sl; do
    if test -r $ac_dir/lib${x_direct_test_library}.$ac_extension; then
      ac_x_libraries=$ac_dir
      break 2
    fi
  done
done])
fi # $ac_x_libraries = NO
])

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

		KDE_PATH_X_DIRECT
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
