#ifndef _CONFIG_H_
#define _CONFIG_H_

@TOP@

/* Define this, if you have libreadline */
#undef HAVE_LIBREADLINE

/* Define this, if you have libhistory */
#undef HAVE_LIBHISTORY

/* Define if the C++ compiler supports BOOL */
#undef HAVE_BOOL

/* The package version */
#undef VERSION 

/* The package name */
#undef PACKAGE

/* Define this to your mtab's path */
#undef MTAB_PATH

/* Define this to your temporary mtab's path */
#undef MTAB_TMP

/* Define this this if you want to prevent plpnfsd from updating mtab */
#undef DONT_UPDATE_MTAB

/* Define this to your serial device node */
#undef DDEV

/* Define this to your serial device speed */
#undef DSPEED

/* Define this to the TCP port ncpd should listen on */
#undef DPORT

/* Define this to your default drive on your Psion */
#undef DDRIVE

/* Define this to your default directory on your Psion */
#undef DBASEDIR

/* Define this to your default mountpoint for plpnfsd */
#undef DMOUNTPOINT

/* Define this to enable debugging code */
#undef DEBUG

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

/* Disable assert macro if DEBUG is not defined */
#ifndef DEBUG
# define NDEBUG
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

