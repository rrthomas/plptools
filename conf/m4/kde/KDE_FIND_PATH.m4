dnl KDE_FIND_PATH(programm-name, variable-name, list of directories,
dnl	if-not-found, test-parameter)
AC_DEFUN(KDE_FIND_PATH,
[
   AC_MSG_CHECKING([for $1])
   kde_cache=`echo $1 | sed 'y%./+-%__p_%'`

   AC_CACHE_VAL(kde_cv_path_$kde_cache,
   [
     kde_cv_path="NONE"
     if test -n "$$2"; then
        kde_cv_path="$$2";
     else
	dirs="$3"
	kde_save_IFS=$IFS
	IFS=':'
	for dir in $PATH; do
	  dirs="$dirs $dir"
        done
	IFS=$kde_save_IFS

        for dir in $dirs; do
	  if test -x "$dir/$1"; then
	    if test -n "$5"
	    then
              evalstr="$dir/$1 $5 2>&1 "
	      if eval $evalstr; then
                kde_cv_path="$dir/$1"
                break
	      fi
            else
		kde_cv_path="$dir/$1"
                break
	    fi
          fi
	done

     fi

     eval "kde_cv_path_$kde_cache=$kde_cv_path"

   ])

   eval "kde_cv_path=\"`echo '$kde_cv_path_'$kde_cache`\""
   if test -z "$kde_cv_path" || test "$kde_cv_path" = NONE; then
      AC_MSG_RESULT(not found)
      $4
   else
      AC_MSG_RESULT($kde_cv_path)
      $2=$kde_cv_path

   fi
])
