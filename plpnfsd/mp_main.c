#include <OSdefs.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include "nfs_prot.h"
#include "mp.h"
#include "defs.h"
#if defined (__SVR4) || defined(__sgi)
#include <stdlib.h>		/* getenv */
#include <string.h>		/* strcmp */
#endif
#include <unistd.h>		/* getuid */

#include <dirent.h>
#if defined(__NeXT__)
#include <sys/dir.h>
#include <unistd.h>
#define DIRENT struct direct
#else
#define DIRENT struct dirent
#endif


extern void nfs_program_2();

static char
*user, *dir = DDIR;

int gmtoffset, debug, exiting, query_cache = 0;

fattr root_fattr =
{
	NFDIR, 0040500, 1, 0, 0,
	BLOCKSIZE, BLOCKSIZE, FID, 1, FID, 1,
	{0, 0},
	{0, 0},
	{0, 0}
};

#if defined(hpux) || defined(__SVR4) || defined(__sgi)
void
usleep(usec)
int usec;
{
	struct timeval t;

	t.tv_sec = (long) (usec / 1000000);
	t.tv_usec = (long) (usec % 1000000);
	select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &t);
}

#endif				/* hpux */

int
mp_main(ac, av)
int ac;
char *av[];
{
	struct passwd *pw;
	struct timeval tv;
	struct timezone tz;
	p_inode *rp;
	nfs_fh root_fh;
	DIR *dirp;
	DIRENT *diep;
	int i;


	if (!(user = (char *) getenv("USER")))
		user = (char *) getenv("logname");

	while (ac > 1) {
		if (!strcmp(av[1], "-v")) {
			debug++;
		} else if (!strcmp(av[1], "-dir")) {
			dir = av[2];
			ac--;
			av++;
		} else if (!strcmp(av[1], "-user") || !strcmp(av[1], "-u")) {
			user = av[2];
			ac--;
			av++;
		} else {
			printf("p3nfsd version %s\n", VERSION);
			printf("Usage: p3nfsd [-dir directory] [-user username]\n");
			printf("              [-wakeup] [-v] [-]\n");
			printf("Defaults: -dir %s -user %s\n", dir, user);
			return 1;
		}
		ac--;
		av++;
	}

	if (user && *user) {
		if (!(pw = getpwnam(user))) {
			fprintf(stderr, "User %s not found.\n", user);
			return 1;
		}
	} else if (!(pw = getpwuid(getuid()))) {
		fprintf(stderr, "You don't exist, go away!\n");
		return 1;
	}
	if (getuid() && pw->pw_uid != getuid()) {
		fprintf(stderr, "%s? You must be kidding...\n", user);
		return 1;
	}
	root_fattr.uid = pw->pw_uid;
	root_fattr.gid = pw->pw_gid;
	endpwent();

	gettimeofday(&tv, &tz);
#ifndef __SVR4
	gmtoffset = -tz.tz_minuteswest * 60;
#else
	tzset();
	gmtoffset = -timezone;
#endif

	printf("plpnfsd: version %s, mounting on %s\n", VERSION, dir);


	/* Check if mountdir is empty (or else you can overmount e.g /etc)
	   It is done here, because exit hangs, if hardware flowcontrol is
	   not present. Bugreport Nov 28 1996 by Olaf Flebbe */
	if (!(dirp = opendir(dir))) {
		perror(dir);
		return 1;
	}
	i = 0;
	while ((diep = readdir(dirp)) != 0)
		if (strcmp(diep->d_name, ".") && strcmp(diep->d_name, ".."))
			i++;
	closedir(dirp);
	if (i) {
		fprintf(stderr, "Sorry, directory %s is not empty, exiting.\n", dir);
		return 1;
	}
	rp = get_nam("");
	inode2fh(rp->inode, root_fh.data);
	root_fattr.fileid = rp->inode;
	root_fattr.atime.seconds = root_fattr.mtime.seconds =
	    root_fattr.ctime.seconds = tv.tv_sec;

	mount_and_run(dir, nfs_program_2, &root_fh);
	return 0;
}
