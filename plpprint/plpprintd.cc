/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stream.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ppsocket.h>
#include <wprt.h>

#define _GNU_SOURCE
#include <getopt.h>

#define TEMPLATE "plpprint_XXXXXX"

char *spooldir = "/var/spool/plpprint";
char *printcmd = "lpr -Ppsion";
wprt *wPrt;
bool serviceLoop;
bool debug = false;
int verbose = 0;

#define alloc_print(p)                                 \
do {                                                   \
    /* Guess we need no more than 100 bytes. */        \
    int n, size = 100;                                 \
    va_list ap;                                        \
    if ((p = (char *)malloc(size)) == NULL)            \
	return 0;                                      \
    while (1) {                                        \
	/* Try to print in the allocated space. */     \
	va_start(ap, fmt);                             \
	n = vsnprintf(p, size, fmt, ap);               \
	va_end(ap);                                    \
	/* If that worked, return the string. */       \
	if (n > -1 && n < size)                        \
	    break;                                     \
	/* Else try again with more space. */          \
	if (n > -1)    /* glibc 2.1 */                 \
	    size = n+1; /* precisely what is needed */ \
	else           /* glibc 2.0 */                 \
	    size *= 2;  /* twice the old size */       \
	if ((p = (char *)realloc(p, size)) == NULL)    \
	    return 0;                                  \
    }                                                  \
} while (0)

int
debuglog(char *fmt, ...)
{
    char *buf;
    alloc_print(buf);
    if (debug)
	cout << buf << endl;
    else
	syslog(LOG_DEBUG, buf);
    free(buf);
    return 0;
}

int
errorlog(char *fmt, ...)
{
    char *buf;
    alloc_print(buf);
    if (debug)
	cerr << buf << endl;
    else
	syslog(LOG_ERR, buf);
    free(buf);
    return 0;
}

int
infolog(char *fmt, ...)
{
    char *buf;
    alloc_print(buf);
    if (debug)
	cout << buf << endl;
    else
	syslog(LOG_INFO, buf);
    free(buf);
    return 0;
}

static void
convert_job(const char *jobname)
{
    // ... To be done ...
    unlink(jobname);
}

static void
service_loop()
{
    serviceLoop = true;
    while (serviceLoop) {
	bool spoolOpen = false;
	bufferStore c;
	int fd;
	char *jname =
	    (char *)malloc(strlen(spooldir) +
			   strlen(TEMPLATE) + 2);

	while (1) {
	    /* Job loop */
	    c.init();
	    if (wPrt->getData(c) != rfsv::E_PSI_GEN_NONE) {
		if (spoolOpen) {
		    unlink(jname);
		    close(fd);
		    errorlog("Job aborted");
		}
		free(jname);
		return;
	    }
	    if ((c.getLen() == 15) && (c.getWord(0) == 0x2a2a)) {
		sprintf(jname, "%s/%s", spooldir, TEMPLATE);
		if ((fd = mkstemp(jname)) != -1) {
		    debuglog("Receiving new job %s", jname);
		    write(fd, c.getString(0), c.getLen());
		    spoolOpen = true;
		} else
		    errorlog("Could not create spool file.");
	    } else {
		if (spoolOpen)
		    write(fd, c.getString(0), c.getLen());
		if (c.getWord(0) == 0xffff)
		    break;
	    }
	}
	if (spoolOpen) {
	    close(fd);
	    spoolOpen = false;
	    debuglog("Job received, start conversion ...");
	    convert_job(jname);
	}
	free(jname);
    }
}

static void
help() {
    cout <<
	"Options of plpprintd:\n"
	"\n"
	" -d, --debug        Debugging, do not fork.\n"
	" -h, --help         Display this text.\n"
	" -v, --verbose      Increase verbosity.\n"
	" -V, --version      Print version and exit.\n"
	" -p, --port=NUM     Connect to port NUM.\n"
	" -s, --spooldir=DIR Specify spooldir DIR.\n"
	" -c, --printcmd=CMD Specify print command.\n";
}

static void
usage() {
    cerr << "Usage: plpprintd [OPTIONS]" << endl
	 << "Use --help for more information" << endl;
}

static struct option opts[] = {
    {"debug",    no_argument,       0, 'd'},
    {"help",     no_argument,       0, 'h'},
    {"version",  no_argument,       0, 'V'},
    {"verbose",  no_argument,       0, 'v'},
    {"port",     required_argument, 0, 'p'},
    {"spooldir", required_argument, 0, 's'},
    {"printcmd", required_argument, 0, 'c'},
    {NULL,       0,                 0,  0 }
};

int
main(int argc, char **argv)
{
    ppsocket *skt;
    int status = 0;
    int sockNum = DPORT;
    int ret = 0;
    int c;

    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != 0L)
	sockNum = ntohs(se->s_port);

    while (1) {
	c = getopt_long(argc, argv, "dhVvp:s:c:", opts, NULL);
	if (c == -1)
	    break;
	switch (c) {
	    case '?':
		usage();
		return -1;
	    case 'd':
		debug = true;
		break;
	    case 'v':
		verbose++;
		break;
	    case 'V':
		cout << "plpprintd Version " << VERSION << endl;
		return 0;
	    case 'h':
		help();
		return 0;
	    case 'p':
		sscanf(optarg, "%hd", &sockNum);
		break;
	    case 's':
		spooldir = strdup(optarg);
		break;
	    case 'c':
		printcmd = strdup(optarg);
		break;
	}
    }
    if (optind < argc) {
	usage();
	return -1;
    }

    skt = new ppsocket();
    if (!skt->connect(NULL, sockNum)) {
	cout << _("plpprintd: could not connect to ncpd") << endl;
	return 1;
    }
    if (!debug)
	ret = fork();
    switch (ret) {
	case 0:
	    /* child */
	    setsid();
	    chdir("/");
	    if (!debug) {
		openlog("plpprintd", LOG_PID|LOG_CONS, LOG_DAEMON);
		int devnull =
		    open("/dev/null", O_RDWR, 0);
		if (devnull != -1) {
		    dup2(devnull, STDIN_FILENO);
		    dup2(devnull, STDOUT_FILENO);
		    dup2(devnull, STDERR_FILENO);
		    if (devnull > 2)
			close(devnull);
		}
	    }
	    infolog("started, waiting for requests.\n");
	    serviceLoop = true;
	    while (serviceLoop) {
		wPrt = new wprt(skt);
		if (wPrt) {
		    Enum<rfsv::errs> ret;
		    ret = wPrt->initPrinter();
		    if (ret == rfsv::E_PSI_GEN_NONE)
			service_loop();
		    else
			debuglog("plpprintd: could not connect: %s",
				 ret.toString().c_str());
		    delete wPrt;
		} else {
		    errorlog("plpprintd: Could not create wprt object");
		    exit(1);
		}
	    }
	    break;
	case -1:
	    cerr << "plpprintd: fork failed" << endl;
	    return 1;
	default:
	    /* parent */
	    break;
    }
    return 0;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
