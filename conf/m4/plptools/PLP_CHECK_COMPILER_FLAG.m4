AC_DEFUN([PLP_CHECK_COMPILER_FLAG],
[
dnl AC_REQUIRE([AC_CHECK_COMPILERS]) <- breaks with autoconf 2.50
AC_MSG_CHECKING(whether $CXX supports -$1)
plp_cache=`echo $1 | sed 'y%.=/+-%___p_%'`
AC_CACHE_VAL(plp_cv_prog_cxx_$plp_cache,
[
echo 'int main() { return 0; }' >conftest.cc
eval "plp_cv_prog_cxx_$plp_cache=no"
if test -z "`$CXX -$1 -c conftest.cc 2>&1`"; then
  if test -z "`$CXX -$1 -o conftest conftest.o 2>&1`"; then
    eval "plp_cv_prog_cxx_$plp_cache=yes"
  fi
fi
rm -f conftest*
])
if eval "test \"`echo '$plp_cv_prog_cxx_'$plp_cache`\" = yes"; then
 AC_MSG_RESULT(yes)
 :
 $2
else
 AC_MSG_RESULT(no)
 :
 $3
fi
])

