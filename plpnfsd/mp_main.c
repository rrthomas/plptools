#include <OSdefs.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
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

int debug, exiting, query_cache = 0;

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
debuglog(char *fmt, ...)
{
	va_list ap;
	char *buf;

	if (!debug)
		return 0;
	buf = (char *)malloc(1024);
	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	syslog(LOG_DEBUG, buf);
	free(buf);
	va_end(ap);
	return 0;
}

int
errorlog(char *fmt, ...)
{
	va_list ap;
	char *buf = (char *)malloc(1024);

	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	va_end(ap);
	syslog(LOG_ERR, buf);
	free(buf);
	return 0;
}

int
infolog(char *fmt, ...)
{
	va_list ap;
	char *buf = (char *)malloc(1024);

	va_start(ap, fmt);
	vsnprintf(buf, 1024, fmt, ap);
	syslog(LOG_INFO, buf);
	free(buf);
	va_end(ap);
	return 0;
}

int
mp_main(int verbose, char *dir, char *user)
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

	debug = verbose;
	if (debug)
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
	openlog("plpnfsd", LOG_PID|LOG_CONS, LOG_DAEMON);
	rp = get_nam("");
	inode2fh(rp->inode, root_fh.data);
	root_fattr.fileid = rp->inode;
	root_fattr.atime.seconds = root_fattr.mtime.seconds =
	    root_fattr.ctime.seconds = tv.tv_sec;

	mount_and_run(dir, nfs_program_2, &root_fh);
	return 0;
}
