dnl
dnl Autoconf check for ccmalloc lib
dnl
dnl Adds an option --with-ccmalloc[=DIR] to your configure script
dnl
dnl Checks for libccmalloc and wrappers either in a specified dir or
dnl in the default dirs /usr/lib /lib and /usr/local/lib
dnl
dnl If found, sets two variables:
dnl
dnl   LIBCCMALLOC_CC  link-commands to use for linking C projects
dnl   LIBCCMALLOC_CXX link-commands to use for linking C++ projects
dnl
dnl Example with autoconf:
dnl
dnl   In your Makefile.in, put something like this:
dnl
dnl     LDFLAGS=$(LIBS) @LIBCCMALLOC_CC@
dnl
dnl Example with automake:
dnl
dnl   In your Makefile.am, put something like this:
dnl
dnl     myprog_LDADD = $(LIBCCMALLOC_CC)
dnl
AC_DEFUN(AC_CHECK_CCMALLOC,[
  LIBCCMALLOC_CXX=
  LIBCCMALLOC_CC=
  AC_ARG_WITH(ccmalloc,
    [  --with-ccmalloc[=DIR]   link against ccmalloc leak-checking lib],
    [
      search_dirs="/usr/lib /lib /usr/local/lib"
      case "${withval}" in
        yes)
          ccm_dirs=${search_dirs}
          ;;
        no)
          ccm_dirs=
          ;;
        *)
          ccm_dirs="${withval} ${search_dirs}"
          ;;
      esac
      AC_MSG_CHECKING(for libccmalloc)
      save_LIBS="$LIBS"
      AC_LANG_SAVE
      AC_LANG_C
      ccm_found=NO
      for d in ${ccm_dirs} ; do
        LIBS="$d/ccmalloc-gcc.o -L$d -lccmalloc -ldl"
        AC_TRY_LINK_FUNC(ccmalloc_report,ccm_found=$d ; break)
      done
      if test "${ccm_found}" = "NO" ; then
        AC_MSG_RESULT(not found)
      else
        libccmalloc="-L$ccm_found -lccmalloc -ldl"
        LIBCCMALLOC_CC="$LIBS"
        ccm_wrappers=gcc
        AC_LANG_CPLUSPLUS
        ccm_wrapper="$ccm_found/ccmalloc-g++.o"
        LIBS="$ccm_wrapper $libccmalloc"
        AC_TRY_LINK(
          [
            extern "C" {
              void ccmalloc_atexit(void(*)(void));
            };
            void myFoo() {}
          ],
          [ccmalloc_atexit(myFoo);],
          [ccm_wrappers="$ccm_wrappers g++"; LIBCCMALLOC_CXX="$LIBS"]
        )
        test "$ccm_wrappers" == "" && ccm_wrappers=none
        AC_MSG_RESULT([(wrappers: $ccm_wrappers) found])
      fi
      AC_LANG_RESTORE
      LIBS="$save_LIBS"
    ]
  )
  AC_SUBST(LIBCCMALLOC_CC)
  AC_SUBST(LIBCCMALLOC_CXX)
])

dnl
dnl This function is the same like the above, with the extension, that
dnl it adds the content of LIBCCMALLOC_CC to LIBS globally. You can
dnl use it, if your project uses C sources only.
dnl
AC_DEFUN(AC_AUTO_CCMALLOC_CC,[
  AC_REQUIRE([AC_CHECK_CCMALLOC])
  LIBS="$LIBS $LIBCCMALLOC_CC"
])

dnl
dnl This function is the same like the above, with the extension, that
dnl it adds the content of LIBCCMALLOC_CXX to LIBS globally. You can
dnl use it, if your project uses C++ sources only.
dnl
AC_DEFUN(AC_AUTO_CCMALLOC_CXX,[
  AC_REQUIRE([AC_CHECK_CCMALLOC])
  LIBS="$LIBS $LIBCCMALLOC_CXX"
])

