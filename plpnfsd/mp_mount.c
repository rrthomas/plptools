/* $Id$
 *
 * Original version of this file from p3nfsd-5.4 by
 * Rudolf Koenig (rfkoenig@immd4.informatik.uni-erlangen.de)
 *
 * Modifications for plputils by Fritz Elfert <felfert@to.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(__SVR4) || defined(__sgi) || defined(linux)
#include <stdlib.h>
#include <unistd.h>
#include <string.h>		/* strdup */
#ifndef linux
extern int inet_addr(char *);
#endif
#define PORTMAP			/* I need svcudp_create in rpc/svc_soc.h, sol 2.4 */
#endif

#include <OSdefs.h>
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>

#ifdef __sgi
#include <bstring.h>
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <sys/param.h>		/* Needs NGROUPS for mount.h */
#define umount(dir) unmount(dir, 1)
#endif

#if defined(__NeXT__)
#include <libc.h>
#include <unistd.h>
#include <nfs/nfs_mount.h>
#define umount(dir) unmount(dir)
#endif


static char nfshost[128];

#define NFSCLIENT
#ifdef __FreeBSD__
#define NFS
#endif				/* __FreeBSD__ */
#include <sys/mount.h>

#ifdef __FreeBSD__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <rpc/rpc.h>
#include <nfs/rpcv2.h>
#if __FreeBSD_version >= 300001
#include <nfs/nfs.h>
#endif
#endif				/* __FreeBSD__ */

#include <rpc/rpc.h>
#include "nfs_prot.h"

#ifdef linux
#ifndef __GLIBC__
#include <linux/fs.h>		/* struct nfs_mount_data */
#endif
#include "linux-misc.h"		/* struct nfs_mount_data */
#include <arpa/inet.h>		/* inet_addr() */
#endif
#ifdef _IBMR2
#include "os-aix3.h"
#include "misc-aix3.h"
#include <sys/vmount.h>
#endif

#ifndef DONT_UPDATE_MTAB
#if defined(sun) && defined(__SVR4)
#include <sys/mnttab.h>
#else
#include <mntent.h>
#endif
#endif

#if defined(sun) && defined(__SVR4)	/*gec */
#include <nfs/mount.h>
#include <sys/fstyp.h>
#include <sys/fsid.h>
#include <sys/types.h>
#include <sys/stat.h>
#define PORTMAP
#endif

#if defined(sun) && !defined(__SVR4)
extern int _rpc_dtablesize();
#define umount unmount
#endif

#include <sys/socket.h>
#include <netdb.h>

#include "rfsv_api.h"

#ifdef __sgi
#define vfork fork
#define NFSMNT_NOCTO 0
#endif

#if defined(sun) && defined(__SVR4)
#define setmntent fopen
#define endmntent fclose
#define addmntent putmntent
#define mntent mnttab
#endif

static char *mntdir;		/* where we mounted the psion */

#ifdef __STDC__
static void doexit(void);
#endif

static void
usr1_handler SIGARG
{
	debug = (debug + 1) & 3;
	infolog("Set debug level to %d\n", debug);
};

static void
hup_handler SIGARG
{
	if (debug > 1)
		debuglog("Got HUP signal\n");
	exiting = 5;
};

static void
term_handler SIGARG
{
	if (debug > 1)
		debuglog("Got TERM signal\n");
	exiting = 5;
};

static void doexit() {
#ifndef DONT_UPDATE_MTAB
	FILE *fpin, *fpout;
#if defined(sun) && defined(__SVR4)
	struct mntent entr;
#else
	struct mntent *ent;
#endif
#endif
#ifndef __FreeBSD__
	struct stat statb;
#endif

	exiting--;
	debuglog("Doing exit\n");

#ifdef _IBMR2
	if (stat(mntdir, &statb)) {
		errorlog("stat %s: %m\n", mntdir);
		return;
	}
	debuglog("Next call: uvmount(%d, 0)\n", statb.st_vfs);
	if (uvmount(statb.st_vfs, 0)) {
		errorlog("uvmount: %m\n");
		return;
	}
#else
	if (umount(mntdir)) {
		errorlog("umount %s: %m\n", mntdir);
		return;
	}
#endif

#ifndef DONT_UPDATE_MTAB
	debuglog("unmount succeeded, trying to fix mtab.\n");

	if (!(fpout = setmntent(MTAB_TMP, "w"))) {
		errorlog("%s: %m\n", MTAB_TMP);
		return;
	}
	if (!(fpin = setmntent(MTAB_PATH, "r"))) {
		endmntent(fpout);
		unlink(MTAB_TMP);
		errorlog("%s: %m\n", MTAB_PATH);
		exit(0);
	}
	if (fstat(fileno(fpin), &statb))
		errorlog("fstat: %m\n");
	else
		fchmod(fileno(fpout), statb.st_mode);

#if defined(sun) && defined(__SVR4)
	while (!getmntent(fpin, &entr))
		if (strcmp(entr.mnt_special, nfshost) || strcmp(entr.mnt_mountp, mntdir))
			putmntent(fpout, &entr);
#else
	while ((ent = getmntent(fpin)))
		if (strcmp(ent->mnt_fsname, nfshost) || strcmp(ent->mnt_dir, mntdir))
			addmntent(fpout, ent);
#endif
	endmntent(fpin);
	endmntent(fpout);

	if (rename(MTAB_TMP, MTAB_PATH)) {
		errorlog("%s: %m\n", MTAB_PATH);
		unlink(MTAB_TMP);
	}
#else
	debuglog("no mtab fixing needed\n");
#endif
	infolog("plpnfsd: terminating.\n");

	exit(0);
}

void cache_flush() {
	struct cache *cp;
	struct dcache *dcp;

	/*
	 * Solaris sends blocks in a way which is not very pleasent
	 * for us. It sends blocks 0,1,2,3,4,5,6, then 9,10,11 and
	 * do on. A little bit later block 7 and 8 arrives. This "bit"
	 * is more that 2 seconds, it is about 6 seconds. It occurs,
	 * if we're rewriting a file. We set MAXWRITE to 15, meaning
	 * that we are waiting for 30 seconds to receive the missing
	 * blocks.
	 */

#define MAXWRITE 15

	int doclean = 1;
	for (cp = attrcache; cp; cp = cp->next)
		for (dcp = cp->dcache; dcp; dcp = dcp->next)
			if (dcp->towrite) {
				debuglog("waiting for block %d in %s to write (%d)\n",
					       dcp->offset, get_num(cp->inode)->name, dcp->towrite);
				if (++dcp->towrite <= MAXWRITE)
					doclean = 0;	/* Wait with cleaning */
			}
	if (doclean || force_cache_clean) {
		for (cp = attrcache; cp; cp = cp->next)
			for (dcp = cp->dcache; dcp; dcp = dcp->next)
				if (dcp->towrite)
					errorlog("PLPNFSD WARNING: file %s block at %d not written\n",
					       get_num(cp->inode)->name, dcp->offset);
		clean_cache(&attrcache);
		if (force_cache_clean || ((time(0) - devcache_stamp) > devcache_keep))
			query_cache = 0;	/* clear the GETDENTS "cache". */
		rfsv_closecached();
		force_cache_clean = 0;
	}
}

void
mount_and_run(char *dir, void (*proc)(), nfs_fh *root_fh)
{
	int sock, port, pid, doclean;
	struct sockaddr_in sain;
	int isalive = 0, ret, dtbsize;
	SVCXPRT *nfsxprt;
	int bufsiz = 0xc000;	/* room for a few biods */
#ifdef linux
	struct nfs_mount_data nfs_mount_data;
	int mount_flags;
	int ksock, kport;
	struct sockaddr_in kaddr;
#else
	struct nfs_args nfs_args;
#endif
#ifdef __FreeBSD__
	int mount_flags;
#endif

	sprintf(nfshost, "localhost:/psion");
	bzero((char *) &sain, sizeof(struct sockaddr_in));
#ifdef linux
	bzero(&nfs_mount_data, sizeof(struct nfs_mount_data));
#else
	bzero((char *) &nfs_args, sizeof(struct nfs_args));
#endif

/*** First part: set up the rpc service */
	/* Create udp socket */
	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char *) &bufsiz, sizeof(bufsiz)))
		errorlog("setsockopt: %m\n");

	/* Bind it to a reserved port */
	sain.sin_family = AF_INET;
	sain.sin_addr.s_addr = inet_addr("127.0.0.1");
	for (port = IPPORT_RESERVED - 1; port > IPPORT_RESERVED / 2; port--) {
		sain.sin_port = htons(port);
		if (bind(sock, (struct sockaddr *) &sain, sizeof(sain)) >= 0)
			break;
	}
	if (port <= IPPORT_RESERVED / 2) {
		errorlog("bind to reserved port: %m\n");
		exit(1);
	}
	if ((nfsxprt = svcudp_create(sock)) == 0) {
		errorlog("svcudp_create: %m\n");
		exit(1);
	}
	if (!svc_register(nfsxprt, NFS_PROGRAM, NFS_VERSION, proc, 0)) {
		errorlog("svc_register: %m\n");
		exit(1);
	}
	debuglog("created svc PROG=%d, VER=%d port=%d\n",
		       NFS_PROGRAM, NFS_VERSION, port);


/*** Second part: mount the directory */
#ifdef linux
	/*
	 * Hold your hat! Another odd internet socket coming up. The linux
	 * kernel needs the socket to talk to the nfs daemon.
	 */
	ksock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (ksock < 0) {
		errorlog("Cannot create kernel socket: %m\n");
		exit(1);
	}
	kaddr.sin_family = AF_INET;
	kaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	for (kport = IPPORT_RESERVED - 1; kport > IPPORT_RESERVED / 2; kport--) {
		kaddr.sin_port = htons(kport);
		if (bind(ksock, (struct sockaddr *) &kaddr, sizeof(kaddr)) >= 0)
			break;
	}
	if (kport <= IPPORT_RESERVED / 2) {
		errorlog("bind to reserved port: %m\n");
		exit(1);
	}
	nfs_mount_data.version = NFS_MOUNT_VERSION;
	nfs_mount_data.fd = ksock;
	nfs_mount_data.root = *root_fh;		/* structure copy */
	nfs_mount_data.flags = NFS_MOUNT_INTR | NFS_MOUNT_NOCTO;
	/* NFS_MOUNT_SECURE | NFS_MOUNT_POSIX | NFS_MOUNT_SOFT | NFS_MOUNT_NOAC */
	nfs_mount_data.rsize = PBUFSIZE;
	nfs_mount_data.wsize = PBUFSIZE;
	nfs_mount_data.timeo = 600;
	nfs_mount_data.retrans = 10;	/* default 3 */
	nfs_mount_data.acregmin = 3;	/* default 3 seconds */
	nfs_mount_data.acregmax = 60;	/* default 60 seconds */
	nfs_mount_data.acdirmin = 30;	/* default 30 seconds */
	nfs_mount_data.acdirmax = 60;	/* default 60 seconds */
	nfs_mount_data.addr = sain;	/* structure copy */
	strcpy(nfs_mount_data.hostname, PSIONHOSTNAME);

	if (connect(ksock, (struct sockaddr *) &nfs_mount_data.addr, sizeof(nfs_mount_data.addr)) < 0) {
		errorlog("Cannot connect to plpnfsd: %m\n");
		exit(1);
	}
	mount_flags = MS_MGC_VAL;	/* | MS_SYNC | MS_RDONLY | MS_NOEXEC | MS_NODEV | MS_NOSUID */

#endif

#if defined(sun) && defined (__SVR4)
	{			/*gec */
		struct netbuf myaddr;
		struct stat stb;
		struct knetconfig knc;
		struct netconfig *nc = getnetconfigent("udp");
		if (!nc) {
			extern int errno;
			errorlog("getnetconfigent \"udp\": (errno %d) %m\n", errno);
			exit(1);
		}
		knc.knc_semantics = nc->nc_semantics;
		knc.knc_protofmly = strdup(nc->nc_protofmly);
		knc.knc_proto = strdup(nc->nc_proto);

		if (stat(nc->nc_device, &stb)) {
			extern int errno;
			errorlog("stat \"%s\": (errno %d) %m\n", nc->nc_device, errno);
			exit(1);
		}
		knc.knc_rdev = stb.st_rdev;

		/*freenetconfigent(nc) has the struct been allocated, or is it static ?!? */

		debuglog("%d,%s,%s,%lx (1,inet,udp,0x002c0029)\n", (int) knc.knc_semantics,
			 knc.knc_protofmly, knc.knc_proto, knc.knc_rdev);

		myaddr.maxlen = myaddr.len = sizeof(sain);
		myaddr.buf = (char *) &sain;

		nfs_args.knconf = &knc;
		nfs_args.wsize = PBUFSIZE;
		nfs_args.rsize = PBUFSIZE;
		nfs_args.addr = &myaddr;
		nfs_args.fh = (char *) root_fh;
		nfs_args.retrans = 10;
		/* 1 minute timeout - see below */
		nfs_args.timeo = 600;
		nfs_args.hostname = PSIONHOSTNAME;
		nfs_args.flags = NFSMNT_INT | NFSMNT_HOSTNAME | NFSMNT_NOCTO |
		    NFSMNT_RETRANS | NFSMNT_TIMEO | NFSMNT_WSIZE |
		    NFSMNT_RSIZE | NFSMNT_KNCONF /* | NFSMNT_NOAC */ ;
		/* eventually drop knc-strings now (memory-leak) */
	}
#endif				/* solaris */

/* BSD Derived systems */
#if (defined(sun) && !defined(__SVR4))  || defined(hpux) || defined(__sgi) || defined(__NeXT__)
	nfs_args.addr = &sain;
	nfs_args.fh = (void *) root_fh;
	nfs_args.wsize = PBUFSIZE;
	nfs_args.rsize = PBUFSIZE;
	nfs_args.retrans = 10;
	/* 1 minute timeout - means that we receive double requests in worst case,
	   if the file is longer than 100k. So long for the theory. Since SunOS uses
	   dynamic retransmission (hardwired), nfs_args.timeo is only a hint. :-( */
	nfs_args.timeo = 600;
	nfs_args.hostname = PSIONHOSTNAME;
	nfs_args.flags = NFSMNT_INT | NFSMNT_HOSTNAME | NFSMNT_NOCTO |
	    NFSMNT_RETRANS | NFSMNT_TIMEO | NFSMNT_WSIZE | NFSMNT_RSIZE;
#endif

#ifdef __FreeBSD__
#if __FreeBSD_version >= 300001
	nfs_args.version = NFS_ARGSVERSION;
#endif
	nfs_args.addr = (struct sockaddr *) &sain;
	nfs_args.addrlen = sizeof(sain);
	nfs_args.sotype = SOCK_DGRAM;
	nfs_args.fh = (void *) root_fh;
	nfs_args.fhsize = sizeof(*root_fh);
	nfs_args.wsize = PBUFSIZE;
	nfs_args.rsize = PBUFSIZE;
	nfs_args.retrans = 10;
	/* 1 minute timeout - see below */
	nfs_args.timeo = 600;
	nfs_args.hostname = PSIONHOSTNAME;
	nfs_args.flags =
	    NFSMNT_INT |
	    NFSMNT_SOFT |
	    NFSMNT_RETRANS |
	    NFSMNT_TIMEO |
	    NFSMNT_WSIZE |
	    NFSMNT_RSIZE;
	mount_flags = MNT_NOSUID | MNT_NODEV | MNT_NOEXEC | MNT_NOATIME;
#endif

#ifdef __NetBSD__
	nfs_args.addrlen = sizeof(sain);
	nfs_args.sotype = SOCK_DGRAM;
	nfs_args.maxgrouplist = NGROUPS;
	nfs_args.readahead = 1;
	nfs_args.addr = (struct sockaddr *) &sain;
	nfs_args.fh = (void *) root_fh;
	nfs_args.wsize = PBUFSIZE;
	nfs_args.rsize = PBUFSIZE;
	nfs_args.retrans = 10;
	nfs_args.timeo = 600;
	nfs_args.hostname = PSIONHOSTNAME;
	nfs_args.flags = NFSMNT_INT | NFSMNT_RETRANS | NFSMNT_TIMEO
	    | NFSMNT_NOCONN | NFSMNT_DUMBTIMR | NFSMNT_MYWRITE
	    | NFSMNT_WSIZE | NFSMNT_RSIZE;
#endif

#if defined(_IBMR2)
	nfs_args.addr = sain;
	nfs_args.fh = *(fhandle_t *) root_fh;
	nfs_args.wsize = PBUFSIZE;
	nfs_args.rsize = PBUFSIZE;
	nfs_args.retrans = 0;	/* Shouldn't need bigger retrans since there is
				   only one biod. Somebody prove it ? Nice feature. */
	nfs_args.biods = 1;
	nfs_args.timeo = 150;
	nfs_args.hostname = PSIONHOSTNAME;
	nfs_args.flags = NFSMNT_INT | NFSMNT_HOSTNAME | NFSMNT_BIODS |
	    NFSMNT_RETRANS | NFSMNT_TIMEO | NFSMNT_WSIZE | NFSMNT_RSIZE;
#endif

	mntdir = dir;
	switch (pid = fork()) {
		case 0:
			break;
		case -1:
			errorlog("fork: %m\n");
			exit(1);
		default:
			debuglog("Going to mount...\n");

#if defined(__sgi) || (defined(sun) && defined(__SVR4))
			if (mount("", dir, MS_DATA, "nfs", &nfs_args, sizeof(nfs_args)))
#endif
#if defined(__NetBSD__) || defined(__NeXT__)
			if (mount(MOUNT_NFS, dir, 0, (caddr_t) & nfs_args))
#endif
#ifdef hpux
			if (vfsmount(MOUNT_NFS, dir, 0, &nfs_args))
#endif
#if defined(sun) && !defined(__SVR4)
			if (mount("nfs", dir, M_NEWTYPE, (caddr_t) & nfs_args))
#endif
#ifdef linux
			if (mount("nfs", dir, "nfs", mount_flags, &nfs_mount_data))
#endif
#ifdef _IBMR2
			if (aix3_mount("psion:loc", dir, 0, MOUNT_TYPE_NFS, &nfs_args, "plpnfsd"))
#endif

#ifdef __FreeBSD__
#if __FreeBSD_version >= 300001
			if (mount("nfs", dir, mount_flags, &nfs_args))
#else
			if (mount(MOUNT_NFS, dir, mount_flags, &nfs_args))
#endif
#endif

			{
				errorlog("nfs mount %s: %m\n", dir);
				kill(pid, SIGTERM);
				exit(0);
			} else {
#ifndef DONT_UPDATE_MTAB
				FILE *mfp;
				struct mntent mnt;
#if defined(sun) && defined(__SVR4)	/*gec */
				char tim[32];
#endif

				debuglog("Mount succeded, making mtab entry.\n");

#if defined(sun) && defined(__SVR4)	/*gec */
				mnt.mnt_special = nfshost;
				mnt.mnt_mountp = mntdir;
				mnt.mnt_fstype = "nfs";
				mnt.mnt_mntopts = "hard,intr";	/* dev=??? */
				sprintf(tim, "%ld", time(NULL));	/* umount crashes! without this */
				mnt.mnt_time = tim;
#else
				mnt.mnt_fsname = nfshost;
				mnt.mnt_dir = mntdir;
				mnt.mnt_type = "nfs";
				mnt.mnt_opts = "hard,intr";
				mnt.mnt_freq = mnt.mnt_passno = 0;
#endif

				if ((mfp = setmntent(MTAB_PATH, "a")))
					addmntent(mfp, &mnt);
				else
					errorlog("%s: %m\n", MTAB_PATH);
				endmntent(mfp);
#endif				/* !DONT_UPDATE_MTAB */
			}
			exit(0);
	}

/*** Third part: let's go */
	infolog("to stop the server do \"echo stop > %s/proc/exit\".\n", mntdir);

#if defined(sun) && !defined(__SVR4)
	dtbsize = _rpc_dtablesize();
#else
	dtbsize = FD_SETSIZE;
#endif

	/*
	 * Signal handling, etc. should only happen in one process, so let's
	 * make it this one (the other one might well go away). We really
	 * ought to block off the other signals, but that's OK...
	 */
	signal(SIGUSR1, usr1_handler);
	signal(SIGHUP, hup_handler);
	signal(SIGTERM, term_handler);
	signal(SIGPIPE, SIG_IGN);

	for (;;) {
		fd_set readfd;
		struct timeval tv;

		readfd = svc_fdset;
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		ret = select(dtbsize, &readfd, 0, 0, &tv);
		if (ret > 0)
			svc_getreqset(&readfd);
		if (ret != 0)
			continue;

		/* Do some housekeeping */

		if (exiting)
			doexit();

		cache_flush();

		ret = rfsv_isalive();
		if (isalive) {
			if (!ret) {
				debuglog("Disconnected...\n");
				doclean = 1;
				root_fattr.mtime.seconds = time(0);
			}
		} else {
			if (ret) {
				debuglog("Connected...\n");
				root_fattr.mtime.seconds = time(0);
			}
		}
		isalive = ret;
	}
}
