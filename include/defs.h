#ifndef _defs_h_
#define _defs_h_

/*
 * NFS related stuff
 */
#define MTAB_PATH	"/etc/mtab"
#ifdef linux
# define MTAB_TMP	"/etc/mtab~"
#else
# define MTAB_TMP	"/etc/mtab.p3nfsd"
#endif

#ifdef _IBMR2
# define DONT_UPDATE_MTAB /* The mount table is obtained from the kernel (!?) */
#endif

#define DUSER "root"

#ifndef DDIR
# define DDIR "/mnt/psion"
#endif

#ifndef PSIONHOSTNAME
# define PSIONHOSTNAME "localhost"
#endif

/* See CHANGES for comment */
#ifdef linux
#define NO_WRITE_SELECT
#endif

/*
 * defaults for ncpd
 */
#define DDEV		"/dev/ttyS0"
#define DSPEED		115200
#define DPORT		7501

/* Debugging */

#define PACKET_LAYER_DIAGNOSTICS false
#define LINK_LAYER_DIAGNOSTICS false
// #define SOCKET_DIAGNOSTICS

#include "config.h"
#endif

