/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999  Philip Proudman <philip.proudman@btinternet.com>
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
#include "config.h"
#endif

#include <stdio.h>
#include <string>
#include <stream.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <plpintl.h>

#include "ncp.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "socketchan.h"
#include "iowatch.h"
#include "linkchan.h"
#include "link.h"
#include "packet.h"
#include "log.h"

#define _GNU_SOURCE
#include <getopt.h>

static bool verbose = false;
static bool active = true;
static bool autoexit = false;

static ncp *theNCP = NULL;
static IOWatch iow;
static IOWatch accept_iow;
static ppsocket skt;
static int numScp = 0;
static socketChan *scp[257]; // MAX_CHANNELS_PSION + 1


static RETSIGTYPE
term_handler(int)
{
    cout << "Got SIGTERM" << endl;
    signal(SIGTERM, term_handler);
    active = false;
};

static RETSIGTYPE
int_handler(int)
{
    cout << "Got SIGINT" << endl;
    signal(SIGINT, int_handler);
    active = false;
};

void
checkForNewSocketConnection()
{
    string peer;
    if (accept_iow.watch(5,0) <= 0) {
	return;
    }
    ppsocket *next = skt.accept(&peer, &iow);
    if (next != NULL) {
	next->setWatch(&iow);
	// New connect
	if (verbose)
	    cout << "New socket connection from " << peer << endl;
	if ((numScp >= theNCP->maxLinks()) || (!theNCP->gotLinkChannel())) {
	    bufferStore a;

	    // Give the client time to send it's version request.
	    next->dataToGet(1, 0);
	    next->getBufferStore(a, false);

	    a.init();
	    a.addStringT("No Psion Connected\n");
	    next->sendBufferStore(a);
	    next->closeSocket();
	    if (verbose)
		cout << "rejected" << endl;
	} else
	    scp[numScp++] = new socketChan(next, theNCP);
    }
}

void *
pollSocketConnections(void *)
{
    while (active) {
        iow.watch(0, 10000);
        for (int i = 0; i < numScp; i++) {
	    scp[i]->socketPoll();
	    if (scp[i]->terminate()) {
	        // Requested channel termination
	        delete scp[i];
	        numScp--;
	        for (int j = i; j < numScp; j++)
		    scp[j] = scp[j + 1];
	        i--;
	    }
        }
    }
    return NULL;
}

static void
help()
{
    cout << _(
	"Usage: plpnfsd [OPTIONS]...\n"
	"\n"
	"Supported options:\n"
	"\n"
        " -d, --dontfork          Run in foreground don't fork\n"
	" -h, --help              Display this text.\n"
	" -V, --version           Print version and exit.\n"
	" -e, --autoexit          Exit after device is disconnected.\n"
	" -v, --verbose=LOGCLASS  Enable logging of LOGCLASS events\n"
	"                         Valid log classes are:\n"
	"                           m   - main program\n"
	"                           nl  - NCP protocol log\n"
	"                           nd  - NCP protocol data dump\n"
	"                           ll  - PLP protocol log\n"
	"                           ld  - PLP protocol data dump\n"
	"                           pl  - physical I/O log\n"
	"                           ph  - physical I/O handshake\n"
	"                           pd  - physical I/O data dump\n"
	"                           all - All of the above\n"
	" -s, --serial=DEV        Use serial device DEV.\n"
	" -b, --baudrate=RATE     Set serial speed to BAUD.\n"
	" -p, --port=[HOST:]PORT  Listen on host HOST, port PORT.\n"
	"                         Default for HOST is 127.0.0.1\n"
	"                         Default for PORT is "
	) << DPORT << "\n\n";
}

static void
usage() {
    cerr << _("Try `ncpd --help' for more information") << endl;
}

static struct option opts[] = {
    {"dontfork",   no_argument,       0, 'd'},
    {"autoexit",   no_argument,       0, 'e'},
    {"help",       no_argument,       0, 'h'},
    {"version",    no_argument,       0, 'V'},
    {"verbose",    required_argument, 0, 'v'},
    {"port",       required_argument, 0, 'p'},
    {"serial",     required_argument, 0, 's'},
    {"baudrate",   required_argument, 0, 'b'},
    {NULL,         0,                 0,  0 }
};

static void
parse_destination(const char *arg, const char **host, int *port)
{
    if (!arg)
	return;
    // We don't want to modify argv, therefore copy it first ...
    char *argcpy = strdup(arg);
    char *pp = strchr(argcpy, ':');

    if (pp) {
	// host.domain:400
	// 10.0.0.1:400
	*pp ++= '\0';
	*host = argcpy;
    } else {
	// 400
	// host.domain
	// host
	// 10.0.0.1
	if (strchr(argcpy, '.') || !isdigit(argcpy[0])) {
	    *host = argcpy;
	    pp = 0L;
	} else
	    pp = argcpy;
    }
    if (pp)
	*port = atoi(pp);
}

static void *
link_thread(void *arg)
{
    while (active) {
        // psion
        iow.watch(1, 0);
        if (theNCP->hasFailed()) {
            if (autoexit) {
		active = false;
                break;
	    }
            iow.watch(5, 0);
            if (verbose)
                cout << "ncp: restarting\n";
            theNCP->reset();
        }
    }
    return NULL;
}

int
main(int argc, char **argv)
{
    int pid;
    bool dofork = true;

    int sockNum = DPORT;
    int baudRate = DSPEED;
    const char *host = "127.0.0.1";
    const char *serialDevice = NULL;
    unsigned short nverbose = 0;

    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != 0L)
	sockNum = ntohs(se->s_port);

    while (1) {
	int c = getopt_long(argc, argv, "hdeVb:s:p:v:", opts, NULL);
	if (c == -1)
	    break;
	switch (c) {
	    case '?':
		usage();
		return -1;
	    case 'V':
		cout << _("plpnfsd Version ") << VERSION << endl;
		return 0;
	    case 'h':
		help();
		return 0;
	    case 'v':
		if (!strcmp(optarg, "nl"))
		    nverbose |= NCP_DEBUG_LOG;
		if (!strcmp(optarg, "nd"))
		    nverbose |= NCP_DEBUG_DUMP;
		if (!strcmp(optarg, "ll"))
		    nverbose |= LNK_DEBUG_LOG;
		if (!strcmp(optarg, "ld"))
		    nverbose |= LNK_DEBUG_DUMP;
		if (!strcmp(optarg, "pl"))
		    nverbose |= PKT_DEBUG_LOG;
		if (!strcmp(optarg, "pd"))
		    nverbose |= PKT_DEBUG_DUMP;
		if (!strcmp(optarg, "ph"))
		    nverbose |= PKT_DEBUG_HANDSHAKE;
		if (!strcmp(optarg, "m"))
		    verbose = true;
		if (!strcmp(optarg, "all")) {
		    nverbose = NCP_DEBUG_LOG | NCP_DEBUG_DUMP |
			LNK_DEBUG_LOG | LNK_DEBUG_DUMP |
			PKT_DEBUG_LOG | PKT_DEBUG_DUMP | PKT_DEBUG_HANDSHAKE;
		    verbose = true;
		}
		break;
	    case 'd':
		dofork = 0;
		break;
	    case 'e':
		autoexit = true;
		break;
	    case 'b':
		if (!strcmp(optarg, "auto"))
		    baudRate = -1;
		else
		    baudRate = atoi(optarg);
		break;
	    case 's':
		serialDevice = optarg;
		break;
	    case 'p':
		parse_destination(optarg, &host, &sockNum);
		break;
	}
    }
    if (optind < argc) {
	usage();
	return -1;
    }

    if (serialDevice == NULL) {
	// If started with -e, assume being started from mgetty and
	// use the tty opened by mgetty instead of the builtin default.
	if (autoexit)
	    serialDevice = ttyname(0);
	else
	    serialDevice = DDEV;
    }

    if (dofork)
	pid = fork();
    else
	pid = 0;
    switch (pid) {
	case 0:
	    signal(SIGTERM, term_handler);
	    signal(SIGINT, int_handler);
	    skt.setWatch(&accept_iow);
	    if (!skt.listen(host, sockNum))
		cerr << "listen on " << host << ":" << sockNum << ": "
		     << strerror(errno) << endl;
	    else {
		if (dofork || autoexit) {
		    logbuf dlog(LOG_DEBUG);
		    logbuf elog(LOG_ERR);
		    ostream lout(&dlog);
		    ostream lerr(&elog);
		    cout = lout;
		    cerr = lerr;
		    openlog("ncpd", LOG_CONS|LOG_PID, LOG_DAEMON);
		    syslog(LOG_INFO,
			   "daemon started. Listening at %s:%d, "
			   "using device %s\n", host, sockNum, serialDevice);
		    setsid();
		    chdir("/");
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
		memset(scp, 0, sizeof(scp));
		theNCP = new ncp(serialDevice, baudRate, nverbose);
		if (!theNCP) {
		    cerr << "Could not create NCP object" << endl;
		    exit(-1);
		}
		pthread_t thr_a, thr_b;
		if (pthread_create(&thr_a, NULL, link_thread, NULL) != 0) {
		    cerr << "Could not create Link thread" << endl;
		    exit(-1);
		}
		if (pthread_create(&thr_a, NULL,
				   pollSocketConnections, NULL) != 0) {
		    cerr << "Could not create Socket thread" << endl;
		    exit(-1);
		}
		while (active)
		    checkForNewSocketConnection();
		void *ret;
		pthread_join(thr_a, &ret);
		pthread_join(thr_b, &ret);
		delete theNCP;
	    }
	    skt.closeSocket();
	    break;
	case -1:
	    cerr << "fork: " << strerror(errno) << endl;
	    break;
	default:
	    exit(0);
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
