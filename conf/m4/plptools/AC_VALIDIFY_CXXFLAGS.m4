dnl AC_VALIDIFY_CXXFLAGS checks for forbidden flags the user may have given
AC_DEFUN([AC_VALIDIFY_CXXFLAGS],
[dnl
 AC_REMOVE_FORBIDDEN(CXX, [-fno-rtti -rpath])
 AC_REMOVE_FORBIDDEN(CXXFLAGS, [-fno-rtti -rpath])
])

