#ifndef _defs_h_
#define _defs_h_

#define MTAB_PATH	"/etc/mtab"
#ifdef linux
# define MTAB_TMP	"/etc/mtab~"
#else
# define MTAB_TMP	"/etc/mtab.p3nfsd"
#endif

#define DDEV		"/dev/ttyS0"

#ifdef _IBMR2
# define DONT_UPDATE_MTAB /* The mount table is obtained from the kernel (!?) */
#endif

#define DUSER "root"

#ifndef DDIR
# define DDIR "/psion.stand/mnt"
#endif

#ifndef PSIONHOSTNAME
# define PSIONHOSTNAME "localhost"
#endif

/* See CHANGES for comment */
#ifdef linux
#define NO_WRITE_SELECT
#endif

#include "config.h"
#endif
