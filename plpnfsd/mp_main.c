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

#include <OSdefs.h>
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>
#include "nfs_prot.h"
#include "mp.h"
#if defined (__SVR4) || defined(__sgi) || defined(__NetBSD__)
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
time_t devcache_keep = 120;
time_t devcache_stamp = 0;

fattr root_fattr =
{
    NFDIR, 0040500, 1, 0, 0,
    BLOCKSIZE, BLOCKSIZE, FID, 1, FID, 1,
    {0, 0},
    {0, 0},
    {0, 0}
};

#if defined(hpux) || defined(__SVR4) || defined(__sgi) 
#ifndef sun  
void
usleep(usec)
int usec;
{
    struct timeval t;

    t.tv_sec = (long) (usec / 1000000);
    t.tv_usec = (long) (usec % 1000000);
    select(0, (fd_set *) 0, (fd_set *) 0, (fd_set *) 0, &t);
}
#endif
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
    syslog(LOG_DEBUG, "%s", buf);
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
    syslog(LOG_ERR, "%s", buf);
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
    syslog(LOG_INFO, "%s", buf);
    free(buf);
    va_end(ap);
    return 0;
}

int force_cache_clean = 0;

int set_owner(char *user, int logstdio) {
    struct passwd *pw = NULL;

    if (user && *user) {
	if (!(pw = getpwnam(user))) {
	    if (logstdio)
		fprintf(stderr, "User %s not found.\n", user);
	    else
		errorlog("User %s not found.\n", user);
	    endpwent();
	    return 1;
	}
	if (getuid() && pw->pw_uid != getuid()) {
	    if (logstdio)
		fprintf(stderr, "Only root can set owner to someone other.\n");
	    else
		errorlog("Only root can set owner to someone other.\n");
	    endpwent();
	    return 1;
	}
    } else {
	if (!logstdio) {
	    errorlog("Request to change owner with empty argument.\n");
	    return 1;
	}
	if (!(pw = getpwuid(getuid()))) {
	    fprintf(stderr, "You don't exist, go away!\n");
	    endpwent();
	    return 1;
	}
    }
    if (pw) {
	if ((root_fattr.uid != pw->pw_uid) || (root_fattr.gid != pw->pw_gid)) {
	    struct group *g = getgrgid(pw->pw_gid);
	    char *gname = (g && g->gr_name && *(g->gr_name)) ? g->gr_name : "???";
	    root_fattr.uid = pw->pw_uid;
	    root_fattr.gid = pw->pw_gid;
	    if (logstdio)
		printf("Owner set to %s.%s\n", pw->pw_name, gname);
	    else
		infolog("Owner set to %s.%s\n", pw->pw_name, gname);
	    endgrent();
	    force_cache_clean = 1;
	    cache_flush();
	}
    }
    endpwent();
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


    if (!(pw = getpwuid(getuid()))) {
	fprintf(stderr, "You don't exist, go away!\n");
	return 1;
    }
    if (!user) {
	if (!(user = (char *) getenv("USER")))
	    user = (char *) getenv("logname");
    }
    endpwent();
    if (set_owner(user, 1))
	return 1;
    gettimeofday(&tv, &tz);

    debug = verbose;
    if (debug)
	printf("plpnfsd: version %s, mounting on %s ...\n", VERSION, dir);

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

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
