AC_DEFUN(KDE_CHECK_FINAL,
[
  AC_ARG_ENABLE(final, [  --enable-final          build size optimized apps (needs lots of memory)],
	kde_use_final=yes, kde_use_final=no, kde_use_final=no)

  if test "x$kde_use_final" = "xyes"; then
      KDE_USE_FINAL_TRUE=""
      KDE_USE_FINAL_FALSE="#"
      KDE_CHECK_REPO
   else
      KDE_USE_FINAL_TRUE="#"
      KDE_USE_FINAL_FALSE=""
  fi
  AC_SUBST(KDE_USE_FINAL_TRUE)
  AC_SUBST(KDE_USE_FINAL_FALSE)

  AC_ARG_ENABLE(closure, [  --disable-closure       don't delay template instantiation],
  	kde_use_closure=$enableval, kde_use_closure=yes)

  if test "x$kde_use_closure" = "xyes"; then
       KDE_USE_CLOSURE_TRUE=""
       KDE_USE_CLOSURE_FALSE="#"
#       CXXFLAGS="$CXXFLAGS $REPO"
  else
       KDE_USE_CLOSURE_TRUE="#"
       KDE_USE_CLOSURE_FALSE=""
  fi
  AC_SUBST(KDE_USE_CLOSURE_TRUE)
  AC_SUBST(KDE_USE_CLOSURE_FALSE)
])
