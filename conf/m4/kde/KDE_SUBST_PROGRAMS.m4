AC_DEFUN([KDE_SUBST_PROGRAMS],
[AC_REQUIRE([AC_CREATE_KFSSTND])dnl

        kde_default_bindirs="/usr/bin /usr/local/bin /opt/local/bin /usr/X11R6/bin /opt/kde/bin /opt/kde2/bin /usr/kde/bin /usr/local/kde/bin"
        if test -n "$KDEDIRS"; then
           kde_save_IFS=$IFS
           IFS=:
           for dir in $KDEDIRS; do
                kde_default_bindirs="$dir/bin $kde_default_bindirs "
           done
           IFS=$kde_save_IFS
        fi
        kde_default_bindirs="$kde_exec_prefix/bin $kde_prefix/bin $kde_default_bindirs"
        KDE_FIND_PATH(dcopidl, DCOPIDL, [$kde_default_bindirs], [KDE_MISSING_PROG_ERROR(dcopidl)])
        KDE_FIND_PATH(dcopidl2cpp, DCOPIDL2CPP, [$kde_default_bindirs], [KDE_MISSING_PROG_ERROR(dcopidl2cpp)])
        KDE_FIND_PATH(mcopidl, MCOPIDL, [$kde_default_bindirs], [KDE_MISSING_PROG_ERROR(mcopidl)])
        KDE_FIND_PATH(kdb2html, KDB2HTML, [$kde_default_bindirs], [KDE_MISSING_PROG_ERROR(kdb2html)])
        KDE_FIND_PATH(artsc-config, ARTSCCONFIG, [$kde_default_bindirs], [KDE_MISSING_PROG_ERROR(artsc-config)])
        KDE_FIND_PATH(kde-config, KDECONFIG, [$kde_default_bindirs], [KDE_MISSING_PROG_ERROR(kde-config)])

        DCOP_DEPENDENCIES='$(DCOPIDL)'
        AC_SUBST(DCOPIDL)
        AC_SUBST(DCOPIDL2CPP)
        AC_SUBST(DCOP_DEPENDENCIES)
        AC_SUBST(MCOPIDL)
        AC_SUBST(KDB2HTML)
        AC_SUBST(ARTSCCONFIG)
        AC_SUBST(KDECONFIG)

        if test -x "$KDECONFIG"; then # it can be "compiled"
          kde_libs_prefix=`$KDECONFIG --prefix`
          if test -z "$kde_libs_prefix" || test ! -x "$kde_libs_prefix"; then
               AC_MSG_ERROR([$KDECONFIG --prefix outputed the non existant prefix '$kde_libs_prefix' for kdelibs.
                          This means it has been moved since you installed it.
                          This won't work. Please recompile kdelibs for the new prefix.
                          ])
           fi
           kde_libs_htmldir=`$KDECONFIG --install html --expandvars`
        else
           kde_libs_prefix='$(kde_prefix)'
           kde_libs_htmldir='$(kde_htmldir)'
        fi
        AC_SUBST(kde_libs_prefix)
        AC_SUBST(kde_libs_htmldir)
])dnl

