dnl ------------------------------------------------------------------------
dnl Find the meta object compiler in the PATH, in $QTDIR/bin, and some
dnl more usual places
dnl ------------------------------------------------------------------------
dnl
AC_DEFUN(KDE_MOC_ERROR_MESSAGE,
[
	AC_MSG_ERROR([Could not find meta object compiler (moc)])
])

AC_DEFUN(AC_PATH_QT_MOC,
[
   KDE_FIND_PATH(moc, MOC, [$ac_qt_bindir $QTDIR/bin $QTDIR/src/moc \
	    /usr/bin /usr/X11R6/bin /usr/lib/qt/bin /usr/lib/qt2/bin \
	    /usr/lib/qt3/bin /usr/local/qt/bin], [KDE_MOC_ERROR_MESSAGE])

   if test -z "$MOC"; then
     if test -n "$ac_cv_path_moc"; then
       eval "$ac_cv_path_moc --help 2>&1 | sed -e '1q' | grep Qt" > cfout.$$
     fi
     echo "configure:__oline__: tried to call $ac_cv_path_moc --help 2>&1 | sed -e '1q' | grep Qt" >&AC_FD_CC
     echo "configure:__oline__: moc output:" >&AC_FD_CC
     cat cfout.$$ >&AC_FD_CC

     if test -s cfout.$$; then
       rm -f cfout.$$
     else
       KDE_MOC_ERROR_MESSAGE
    fi
    rm -f cfout.$$
   fi

   AC_SUBST(MOC)
])
