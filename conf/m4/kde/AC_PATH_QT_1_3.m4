dnl ------------------------------------------------------------------------
dnl Try to find the Qt headers and libraries.
dnl $(QT_LDFLAGS) will be -Lqtliblocation (if needed)
dnl and $(QT_INCLUDES) will be -Iqthdrlocation (if needed)
dnl ------------------------------------------------------------------------
dnl
dnl
AC_DEFUN(AC_TRY_PATH_QT,[
	AC_REQUIRE([K_PATH_X])
	AC_REQUIRE([KDE_USE_QT])

	LIBQT="-l$1"
	if test $kde_qtver = 2; then
		AC_REQUIRE([AC_FIND_PNG])
		LIBQT="$LIBQT $LIBPNG"
	fi

	echo -n "($2) "

	LIBQT="$LIBQT $X_PRE_LIBS -lXext -lX11 $LIBSOCKET"
	ac_qt_includes=NO ac_qt_libraries=NO ac_qt_bindir=NO
	qt_libraries=""
	qt_includes=""
	kde_qt_libs_given=no

	AC_ARG_WITH(qt-dir,
		[  --with-qt-dir=DIR       where the root of Qt is installed ],
		[  ac_qt_includes="$withval"/include
		   ac_qt_libraries="$withval"/lib
		   ac_qt_bindir="$withval"/bin
		]
	)

	AC_ARG_WITH(qt-includes,
		[  --with-qt-includes=DIR  where the Qt includes are. ],
		[ ac_qt_includes="$withval" ]
	)


	AC_ARG_WITH(qt-libraries,
		[  --with-qt-libraries=DIR where the Qt library is installed.],
		[  ac_qt_libraries="$withval"
		   kde_qt_libs_given=yes
		]
	)

	#try to guess Qt locations

	qt_incdirs="$QTINC /usr/lib/qt/include /usr/local/qt/include /usr/include/qt /usr/include /usr/lib/qt3/include /usr/lib/qt2/include /usr/X11R6/include/X11/qt $x_includes"
	test -n "$QTDIR" && qt_incdirs="$QTDIR/include $QTDIR $qt_incdirs"
	qt_incdirs="$ac_qt_includes $qt_incdirs"

	if test "$kde_qtver" = "2"; then
		kde_qt_header=qstyle.h
	else
		kde_qt_header=qglobal.h
	fi

	AC_FIND_FILE($kde_qt_header, $qt_incdirs, qt_incdir)
	ac_qt_includes="$qt_incdir"

	qt_libdirs="$QTLIB /usr/lib/qt/lib /usr/X11R6/lib /usr/lib /usr/local/qt/lib /usr/lib/qt /usr/lib/qt3/lib /usr/lib/qt2/lib $x_libraries"
	test -n "$QTDIR" && qt_libdirs="$QTDIR/lib $QTDIR $qt_libdirs"
	if test ! "$ac_qt_libraries" = "NO"; then
		qt_libdirs="$ac_qt_libraries $qt_libdirs"
	fi

	qt_libdir=NONE
	for dir in $qt_libdirs; do
		try="ls -1 $dir/lib$1.*so"
		if eval $try >/dev/null 2>&1 ; then
			qt_libdir=$dir;
			break;
		else
			echo "tried $dir" >&AC_FD_CC
		fi
	done

	ac_qt_libraries="$qt_libdir"

	AC_LANG_SAVE
	AC_LANG_CPLUSPLUS

	ac_cxxflags_safe="$CXXFLAGS"
	ac_ldflags_safe="$LDFLAGS"
	ac_libs_safe="$LIBS"

	CXXFLAGS="$CXXFLAGS -I$qt_incdir $all_includes"
	LDFLAGS="-L$qt_libdir $all_libraries"
	LIBS="$LIBS $LIBQT"

	KDE_PRINT_QT_PROGRAM

	if AC_TRY_EVAL(ac_link) && test -s conftest; then
		rm -f conftest*
	else
		echo "configure: failed program was:" >&AC_FD_CC
		cat conftest.$ac_ext >&AC_FD_CC
		ac_qt_libraries="NO"
	fi
	rm -f conftest*
	CXXFLAGS="$ac_cxxflags_safe"
	LDFLAGS="$ac_ldflags_safe"
	LIBS="$ac_libs_safe"

	AC_LANG_RESTORE

	if test "$ac_qt_includes" = NO || test "$ac_qt_libraries" = NO; then
		have_qt=no
		ac_qt_notfound=""
		if test "$ac_qt_includes" = NO; then
			if test "$ac_qt_libraries" = NO; then
				ac_qt_notfound="(headers and libraries)";
			else
				ac_qt_notfound="(headers)";
			fi
		else
			ac_qt_notfound="(libraries)";
		fi
	else
		have_qt="yes"
	fi
])


AC_DEFUN(AC_PATH_QT_ANY,[

	AC_MSG_CHECKING([for Qt])
	AC_CACHE_VAL(ac_cv_have_qt,[
		AC_TRY_PATH_QT(qt, single-threaded)
		if test "$have_qt" != yes; then
			AC_TRY_PATH_QT(qt-mt, multi-threaded)
			if test "$have_qt" != yes; then
				AC_MSG_ERROR([Qt ($kde_qt_minversion) $ac_qt_notfound not found. Please check your installation! ]);
			fi
		fi
		ac_cv_have_qt="have_qt=$have_qt"
	])

	eval "$ac_cv_have_qt"
	if test "$have_qt" != yes; then
		AC_MSG_RESULT([$have_qt]);
	else
		ac_cv_have_qt="have_qt=yes \
			ac_qt_includes=$ac_qt_includes ac_qt_libraries=$ac_qt_libraries"
		AC_MSG_RESULT([libraries $ac_qt_libraries, headers $ac_qt_includes])

		qt_libraries="$ac_qt_libraries"
		qt_includes="$ac_qt_includes"
	fi

	if test ! "$kde_qt_libs_given" = "yes"; then
		KDE_CHECK_QT_DIRECT(qt_libraries= ,[])
	fi

	AC_SUBST(qt_libraries)
	AC_SUBST(qt_includes)

	if test "$qt_includes" = "$x_includes" || test -z "$qt_includes"; then
		QT_INCLUDES="";
	else
		QT_INCLUDES="-I$qt_includes"
		all_includes="$QT_INCLUDES $all_includes"
	fi

	if test "$qt_libraries" = "$x_libraries" || test "$qt_libraries" = "/usr/lib" || test -z "$qt_libraries"; then
		QT_LDFLAGS=""
	else
		QT_LDFLAGS="-L$qt_libraries"
		all_libraries="$QT_LDFLAGS $all_libraries"
	fi

	AC_SUBST(QT_INCLUDES)
	AC_SUBST(QT_LDFLAGS)
	AC_PATH_QT_MOC

	LIB_QT='$(LIBQT) $(LIBPNG) -lXext $(LIB_X11) $(X_PRE_LIBS)'
	AC_SUBST(LIB_QT)
])
