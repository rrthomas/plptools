// $Id$
//
//  PLP - An implementation of the PSION link protocol
//
//  Copyright (C) 1999  Philip Proudman
//  Modifications for plptools:
//    Copyright (C) 1999 Fritz Elfert <felfert@to.com>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  e-mail philip.proudman@btinternet.com

#include <stdio.h>
#include <string.h>
#include <stream.h>
#include <stdlib.h>
#include <signal.h>

#include "defs.h"
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

void
checkForNewSocketConnection(ppsocket & skt, int &numScp, socketChan ** scp, ncp * a, IOWatch & iow)
{
	char peer[201];
	ppsocket *next = skt.accept(peer, 200);
	if (next != NULL) {
		// New connect
		if (verbose)
			cout << "New socket connection from " << peer << endl;
		if ((numScp == 7) || (!a->gotLinkChannel())) {
			bufferStore a;
			a.addStringT("No Psion Connected\n");
			next->sendBufferStore(a);
			next->closeSocket();
		} else
			scp[numScp++] = new socketChan(next, a, iow);
	}
}

void
pollSocketConnections(int &numScp, socketChan ** scp)
{
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

void
usage()
{
	cerr << "Usage : ncpd [-V] [-v logclass] [-d] [-e] [-p <port>] [-s <device>] [-b <baudrate>]\n";
	exit(1);
}

int
main(int argc, char **argv)
{
	ppsocket skt;
	IOWatch iow;
	int pid;
	bool dofork = true;
	bool autoexit = false;

	// Command line parameter processing
	int sockNum = DPORT;
	int baudRate = DSPEED;
	const char *serialDevice = DDEV;
	short int nverbose = 0;
	short int pverbose = 0;
	short int lverbose = 0;

	signal(SIGPIPE, SIG_IGN);
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-p") && i + 1 < argc) {
			sockNum = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-s") && i + 1 < argc) {
			serialDevice = argv[++i];
		} else if (!strcmp(argv[i], "-v") && i + 1 < argc) {
			i++;
			if (!strcmp(argv[i], "nl"))
				nverbose |= NCP_DEBUG_LOG;
			if (!strcmp(argv[i], "nd"))
				nverbose |= NCP_DEBUG_DUMP;
			if (!strcmp(argv[i], "ll"))
				lverbose |= LNK_DEBUG_LOG;
			if (!strcmp(argv[i], "ld"))
				lverbose |= LNK_DEBUG_DUMP;
			if (!strcmp(argv[i], "pl"))
				pverbose |= PKT_DEBUG_LOG;
			if (!strcmp(argv[i], "pd"))
				pverbose |= PKT_DEBUG_DUMP;
			if (!strcmp(argv[i], "ph"))
				pverbose |= PKT_DEBUG_HANDSHAKE;
			if (!strcmp(argv[i], "m"))
				verbose = true;
			if (!strcmp(argv[i], "all")) {
				nverbose = NCP_DEBUG_LOG | NCP_DEBUG_DUMP;
				lverbose = LNK_DEBUG_LOG | LNK_DEBUG_DUMP;
				pverbose = PKT_DEBUG_LOG | PKT_DEBUG_DUMP;
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

	if (dofork)
		pid = fork();
	else
		pid = 0;
	switch (pid) {
		case 0:
			if (!skt.listen("127.0.0.1", sockNum))
				cerr << "listen on port " << sockNum << ": " << strerror(errno) << endl;
			else {
				if (dofork) {
					logbuf dlog(LOG_DEBUG);
					logbuf elog(LOG_ERR);
					ostream lout(&dlog);
					ostream lerr(&elog);
					cout = lout;
					cerr = lerr;
					openlog("ncpd", LOG_CONS|LOG_PID, LOG_DAEMON);
				}
				ncp *a = new ncp(serialDevice, baudRate, iow);
				int numScp = 0;
				socketChan *scp[MAX_CHANNEL+1];

				a->setVerbose(nverbose);
				a->setLinkVerbose(lverbose);
				a->setPktVerbose(pverbose);
				iow.addIO(skt.socket());
				while (true) {
					// sockets
					pollSocketConnections(numScp, scp);
					checkForNewSocketConnection(skt, numScp, scp, a, iow);

					// psion
					a->poll();

					if (a->stuffToSend())
						iow.watch(0, 100000);
					else
						iow.watch(1, 0);

					if (a->hasFailed()) {
						if (autoexit)
							break;

						iow.watch(5, 0);
						if (verbose)
							cout << "ncp: restarting\n";
						a->reset();
					}
				}
				delete a;
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
