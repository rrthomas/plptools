AC_DEFUN(AC_CREATE_KFSSTND,
[
AC_MSG_CHECKING([for KDE paths])
kde_result=""

AC_CACHE_VAL(kde_cv_all_paths,
[
  if test -z "$kde_htmldir"; then
    kde_htmldir='\$(kde_prefix)/share/doc/HTML'
  fi
  if test -z "$kde_appsdir"; then
    kde_appsdir='\$(kde_prefix)/share/applnk'
  fi
  if test -z "$kde_icondir"; then
    kde_icondir='\$(kde_prefix)/share/icons'
  fi
  if test -z "$kde_sounddir"; then
    kde_sounddir='\$(kde_prefix)/share/sounds'
  fi
  if test -z "$kde_datadir"; then
    kde_datadir='\$(kde_prefix)/share/apps'
  fi
  if test -z "$kde_locale"; then
    kde_locale='\$(kde_prefix)/share/locale'
  fi
  if test -z "$kde_cgidir"; then
    kde_cgidir='\$(kde_exec_prefix)/cgi-bin'
  fi
  if test -z "$kde_confdir"; then
    kde_confdir='\$(kde_prefix)/share/config'
  fi
  if test -z "$kde_mimedir"; then
    kde_mimedir='\$(kde_prefix)/share/mimelnk'
  fi
  if test -z "$kde_toolbardir"; then
    kde_toolbardir='\$(kde_prefix)/share/toolbar'
  fi
  if test -z "$kde_wallpaperdir"; then
    kde_wallpaperdir='\$(kde_prefix)/share/wallpapers'
  fi
  if test -z "$kde_bindir"; then
    kde_bindir='\$(kde_exec_prefix)/bin'
  fi
  if test -z "$kde_servicesdir"; then
    kde_servicesdir='\$(kde_prefix)/share/services'
  fi
  if test -z "$kde_servicetypesdir"; then
    kde_servicetypesdir='\$(kde_prefix)/share/servicetypes'
  fi

  kde_cv_all_paths="kde_have_all_paths=\"yes\" \
	kde_htmldir=\"$kde_htmldir\" \
	kde_appsdir=\"$kde_appsdir\" \
	kde_icondir=\"$kde_icondir\" \
	kde_sounddir=\"$kde_sounddir\" \
	kde_datadir=\"$kde_datadir\" \
	kde_locale=\"$kde_locale\" \
	kde_cgidir=\"$kde_cgidir\" \
	kde_confdir=\"$kde_confdir\" \
	kde_mimedir=\"$kde_mimedir\" \
	kde_toolbardir=\"$kde_toolbardir\" \
	kde_wallpaperdir=\"$kde_wallpaperdir\" \
	kde_bindir=\"$kde_bindir\" \
	kde_servicesdir=\"$kde_servicesdir\" \
	kde_servicetypesdir=\"$kde_servicetypesdir\" \
	kde_result=defaults"

])

eval "$kde_cv_all_paths"

if test -z "$kde_htmldir" || test -z "$kde_appsdir" ||
   test -z "$kde_icondir" || test -z "$kde_sounddir" ||
   test -z "$kde_datadir" || test -z "$kde_locale"  ||
   test -z "$kde_cgidir"  || test -z "$kde_confdir" ||
   test -z "$kde_mimedir" || test -z "$kde_toolbardir" ||
   test -z "$kde_wallpaperdir" || test -z "$kde_bindir" ||
   test -z "$kde_servicesdir" ||
   test -z "$kde_servicetypesdir" || test "$kde_have_all_paths" != "yes"; then
  kde_have_all_paths=no
  AC_MSG_ERROR([configure could not run a little KDE program to test the environment.
Since it had compiled and linked before, it must be a strange problem on your system.
Look at config.log for details. If you are not able to fix this, look at
http://www.kde.org/faq/installation.html or any www.kde.org mirror.
(If you're using an egcs version on Linux, you may update binutils!)
])
else
  rm -f conftest*
  AC_MSG_RESULT($kde_result)
fi

])
