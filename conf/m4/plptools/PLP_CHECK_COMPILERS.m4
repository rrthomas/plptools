AC_DEFUN(PLP_CHECK_COMPILERS,
[
  AC_ARG_ENABLE(debug,[  --enable-debug          enables debug symbols [default=no]],
  [
   if test $enableval = "no"; dnl
     then
       plp_use_debug_code="no"
       plp_use_debug_define=yes
     else
       plp_use_debug_code="yes"
       plp_use_debug_define=no
   fi
  ], 
    [plp_use_debug_code="no"
      plp_use_debug_define=no
  ])

  dnl Just for configure --help
  AC_ARG_ENABLE(dummyoption,[  --disable-debug         disables debug output and debug symbols [default=no]],[],[])

  AC_ARG_ENABLE(strict,[  --enable-strict         compiles with strict compiler options (may not work!)],
   [
    if test $enableval = "no"; then
         plp_use_strict_options="no"
       else
         plp_use_strict_options="yes"
    fi
   ], [plp_use_strict_options="no"])

  AC_ARG_ENABLE(profile,[  --enable-profile        creates profiling infos [default=no]],
    [plp_use_profiling=$enableval],
    [plp_use_profiling="no"]
  )

  dnl this prevents stupid AC_PROG_CC to add "-g" to the default CFLAGS
  CFLAGS=" $CFLAGS"

  AC_PROG_CC 

  if test "$GCC" = "yes"; then
    if test "$plp_use_debug_code" = "yes"; then
      CFLAGS="-g $CFLAGS"
      case $host in
        *-*-linux-gnu)	
          CFLAGS="-Wpointer-arith $CFLAGS"
        ;;
      esac
    else
      CFLAGS="-O2 $CFLAGS"
    fi
    if test "$plp_use_strict_options" = "yes"; then
      CFLAGS="-ansi -Wall -W -pedantic -Wshadow -Wmissing-prototypes -Wwrite-strings -D_XOPEN_SOURCE=500 -D_BSD_SOURCE $CFLAGS"
    fi
  fi

  if test "$plp_use_debug_define" = "yes"; then
    CFLAGS="-DNDEBUG $CFLAGS"
  fi

  case "$host" in
  *-*-sysv4.2uw*) CFLAGS="-D_UNIXWARE $CFLAGS";;
  *-*-sysv5uw7*) CFLAGS="-D_UNIXWARE7 $CFLAGS";;
  esac

  if test "$plp_use_debug_code" = "no"; then
    if test -z "$LDFLAGS" && test "$GCC" = "yes"; then
      LDFLAGS="-s"
    fi
    AC_DEFINE_UNQUOTED(NDEBUG,1,[Define this to disable assert macro])
    LIBDEBUG=""
  else
    AC_DEFINE_UNQUOTED(DEBUG,1,[Define this to enable debugging code])
    LIBDEBUG="--debug"
  fi
  AC_SUBST(LIBDEBUG)

  CXXFLAGS=" $CXXFLAGS"

  AC_PROG_CXX

  if test "$GXX" = "yes"; then
    if test "$plp_use_debug_code" = "yes"; then
      CXXFLAGS="-g -Wpointer-arith -Wmissing-prototypes $CXXFLAGS"

      PLP_CHECK_COMPILER_FLAG(Wno-long-long,[CXXFLAGS="-Wno-long-long $CXXFLAGS"])
      PLP_CHECK_COMPILER_FLAG(Wnon-virtual-dtor,[CXXFLAGS="-Wnon-virtual-dtor $CXXFLAGS"])
      PLP_CHECK_COMPILER_FLAG(fno-builtin,[CXXFLAGS="-fno-builtin $CXXFLAGS"])

      case $host in  dnl
      *-*-linux-gnu)
        CXXFLAGS="-D_XOPEN_SOURCE=500 -D_BSD_SOURCE -Wbad-function-cast -Wcast-align -Wundef $CXXFLAGS"
        ;;
      esac

      if test "$plp_use_strict_options" = "yes"; then
        CXXFLAGS="-ansi -pedantic -W -Wconversion -Wcast-qual -Wwrite-strings -Wshadow -Wcast-align $CXXFLAGS"
      fi
    else
      CXXFLAGS="-O2 $CXXFLAGS"
    fi
  fi

  if test "$plp_use_debug_define" = "yes"; then
    CXXFLAGS="-DNDEBUG $CXXFLAGS"
  fi  

  if test "$plp_use_profiling" = "yes"; then
    PLP_CHECK_COMPILER_FLAG(pg,
    [
      CFLAGS="-pg $CFLAGS"
      CXXFLAGS="-pg $CXXFLAGS"
    ])
  fi
    
  PLP_CHECK_COMPILER_FLAG(fno-exceptions,[CXXFLAGS="$CXXFLAGS -fno-exceptions"])
  PLP_CHECK_COMPILER_FLAG(fno-check-new, [CXXFLAGS="$CXXFLAGS -fno-check-new"])
  PLP_CHECK_COMPILER_FLAG(fexceptions, [USE_EXCEPTIONS="-fexceptions"], USE_EXCEPTIONS=	)
  AC_SUBST(USE_EXCEPTIONS)
  dnl obsolete macro - provided to keep things going
  USE_RTTI=
  AC_SUBST(USE_RTTI)

  case "$host" in
      *-*-irix*)  test "$GXX" = yes && CXXFLAGS="-D_LANGUAGE_C_PLUS_PLUS -D__LANGUAGE_C_PLUS_PLUS $CXXFLAGS" ;;
      *-*-sysv4.2uw*) CXXFLAGS="-D_UNIXWARE $CXXFLAGS";;
      *-*-sysv5uw7*) CXXFLAGS="-D_UNIXWARE7 $CXXFLAGS";;
      *-*-solaris*) 
        if test "$GXX" = yes; then
          libstdcpp=`$CXX -print-file-name=libstdc++.so`
          if test ! -f $libstdcpp; then
             AC_MSG_ERROR([You've compiled gcc without --enable-shared. This doesn't work with plptools. Please recompile gcc with --enable-shared to receive a libstdc++.so])
          fi
        fi
        ;;
  esac

  AC_VALIDIFY_CXXFLAGS

  AC_PROG_CXXCPP

  # the following is to allow programs, that are known to
  # have problems when compiled with -O2
  if test -n "$CXXFLAGS"; then
      plp_safe_IFS=$IFS
      IFS=" "
      NOOPT_CXXFLAGS=""
      for i in $CXXFLAGS; do
        case $i in
          -O*)
                ;;
          *)
                NOOPT_CXXFLAGS="$NOOPT_CXXFLAGS $i"
                ;;
        esac
      done
      IFS=$plp_safe_IFS
  fi

  if test "$plp_use_debug_code" = "yes"; then
    STRIP=true
    AC_SUBST(STRIP)
  fi

  AC_SUBST(NOOPT_CXXFLAGS)
  THREADED_CFLAGS="-D_REENTRANT $CFLAGS"
  THREADED_CXXFLAGS="-D_REENTRANT $CXXFLAGS"
  AC_SUBST(THREADED_CFLAGS)
  AC_SUBST(THREADED_CXXFLAGS)
])

