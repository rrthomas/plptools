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

#include "ncp.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "socketchan.h"
#include "iowatch.h"
#include "linkchan.h"
#include "link.h"
#include "packet.h"
#include "log.h"

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

void
usage()
{
    cerr << "Usage : ncpd [-V] [-v logclass] [-d] [-e] [-p [<host>:]<port>] [-s <device>] [-b <baudrate>]\n";
    exit(1);
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

    // Command line parameter processing
    for (int i = 1; i < argc; i++) {
	if (!strcmp(argv[i], "-p") && i + 1 < argc) {
	    // parse port argument
	    i++;
	    char *pp = strchr(argv[i], ':');
	    if (pp != NULL) {
		// host.domain:400
		// 10.0.0.1:400
		*pp ++= '\0';
		host = argv[i];
	    } else {
		// 400
		// host.domain
		// host
		// 10.0.0.1
		if (strchr(argv[i], '.') || !isdigit(argv[i][0])) {
		    host = argv[i];
		    pp = NULL;
		} else
		    pp = argv[i];
	    }
	    if (pp != NULL)
		sockNum = atoi(pp);
	} else if (!strcmp(argv[i], "-s") && i + 1 < argc) {
	    serialDevice = argv[++i];
	} else if (!strcmp(argv[i], "-v") && i + 1 < argc) {
	    i++;
	    if (!strcmp(argv[i], "nl"))
		nverbose |= NCP_DEBUG_LOG;
	    if (!strcmp(argv[i], "nd"))
		nverbose |= NCP_DEBUG_DUMP;
	    if (!strcmp(argv[i], "ll"))
		nverbose |= LNK_DEBUG_LOG;
	    if (!strcmp(argv[i], "ld"))
		nverbose |= LNK_DEBUG_DUMP;
	    if (!strcmp(argv[i], "pl"))
		nverbose |= PKT_DEBUG_LOG;
	    if (!strcmp(argv[i], "pd"))
		nverbose |= PKT_DEBUG_DUMP;
	    if (!strcmp(argv[i], "ph"))
	        nverbose |= PKT_DEBUG_HANDSHAKE;
	    if (!strcmp(argv[i], "m"))
		verbose = true;
	    if (!strcmp(argv[i], "all")) {
		nverbose = NCP_DEBUG_LOG | NCP_DEBUG_DUMP |
		    LNK_DEBUG_LOG | LNK_DEBUG_DUMP |
		    PKT_DEBUG_LOG | PKT_DEBUG_DUMP | PKT_DEBUG_HANDSHAKE;
		verbose = true;
	    }
	} else if (!strcmp(argv[i], "-b") && i + 1 < argc) {
	    baudRate = atoi(argv[++i]);
	} else if (!strcmp(argv[i], "-d")) {
	    dofork = 0;
	} else if (!strcmp(argv[i], "-e")) {
	    autoexit = true;
	} else if (!strcmp(argv[i], "-V")) {
	    cout << "ncpd version " << VERSION << endl;
	    exit(0);
	} else
	    usage();
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
