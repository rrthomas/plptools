AC_DEFUN([PLP_SET_LIBVERSION],
	[
		AC_REQUIRE([AM_INIT_AUTOMAKE])
		maj=$(echo ${VERSION} | cut -d. -f1)
		min=$(echo ${VERSION} | cut -d. -f2)
		LIBVERSION=${maj}:${min}:${maj}
		AC_SUBST(LIBVERSION)
	]
)
