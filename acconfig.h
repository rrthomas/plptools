#ifndef _CONFIG_H_
#define _CONFIG_H_

@TOP@

/* Define this, if you have sys/time.h */
#undef HAVE_SYS_TIME_H

/* Define this, if libc provides bind_textdomain_codeset */
#undef HAVE_BIND_TEXTDOMAIN_CODESET

/* Define this, if you have libreadline */
#undef HAVE_LIBREADLINE

/* Define if the C++ compiler supports BOOL */
#undef HAVE_BOOL

/* Define this, if you want NLS support */
#undef ENABLE_NLS

/* Define this, if you have catgets */
#undef HAVE_CATGETS

/* Define this, if you have gettext */
#undef HAVE_GETTEXT

/* Define this, if you have stpcpy */
#undef HAVE_STPCPY

/* Define this, if your locale.h defines LC_MESSAGES */
#undef HAVE_LC_MESSAGES

/* The package version */
#undef VERSION 

/* The package name */
#undef PACKAGE

/* Define this to enable debugging code */
#undef DEBUG

/* Define this, if sys/int_types.h is on your system */
#undef HAVE_SYS_INT_TYPES_H

/* Define this, if stdint.h is on your system */
#undef HAVE_STDINT_H

/* Define this, if your int-types have an underscore after the first u/s */
#undef GNU_INTTYPES

/* Define to 401 or 402, depending on readline version 4.1 or 4.2 */
#define READLINE_VERSION 401

@BOTTOM@

/* Some reasonable defaults */

#ifndef PSIONHOSTNAME
# define PSIONHOSTNAME "localhost"
#endif

/* misc tweaks */

#ifdef _IBMR2
# undef DONT_UPDATE_MTAB
# define DONT_UPDATE_MTAB /* The mount table is obtained from the kernel (!?) */
#endif

#ifndef HAVE_BOOL
# ifndef bool
#  define bool int
# endif
# ifndef true
#  define true 1
# endif
# ifndef false
#  define false 0
# endif
#endif

#endif /* _CONFIG_H_ */

