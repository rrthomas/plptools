AC_DEFUN([KDE_USE_QT],
[
if test -z "$1"; then
  kde_qtver=2
else
  kde_qtver=$1
fi

if test -z "$2"; then
  if test $kde_qtver = 2; then
    kde_qt_minversion=">= 19991109"
   else
    kde_qt_minversion=">= 1.42 and < 2.0"
  fi
else
   kde_qt_minversion=$2
fi

if test -z "$3"; then
    if test $kde_qtver = 2; then
    kde_qt_verstring="QT_VERSION >= 200"
   else
    kde_qt_verstring="QT_VERSION >= 142 && QT_VERSION < 200"
  fi
else
   kde_qt_verstring=$3
fi
])
