//
//  PLP - An implementation of the PSION link protocol
//
//  Copyright (C) 1999  Philip Proudman
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

#include "defs.h"
#include "ncp.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "socketchan.h"
#include "iowatch.h"
#include "linkchan.h"
#include "link.h"
#include "packet.h"

void
checkForNewSocketConnection(ppsocket & skt, int &numScp, socketChan ** scp, ncp * a, IOWatch & iow)
{
	char peer[201];
	ppsocket *next = skt.accept(peer, 200);
	if (next != NULL) {
		// New connect
		cout << "New socket connection from " << peer << endl;
		if ((numScp == 7) || (!a->gotLinkChannel())) {
			bufferStore a;
			a.addStringT("No psion ncp channel free");
			next->sendBufferStore(a);
			sleep(1);
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
resetSocketConnections(int &numScp, socketChan ** scp, ncp * a)
{
	for (int i = 0; i < numScp; i++) {
		if (scp[i]->isConnected()) {
			cout << "Killing\n";
			delete scp[i];
			numScp--;
			for (int j = i; j < numScp; j++)
				scp[j] = scp[j + 1];
			i--;
		} else {
			scp[i]->newNcpController(a);
			if (scp[i]->getNcpConnectName() != NULL) {
				cout << "Connecting\n";
				scp[i]->ncpConnect();
			} else
				cout << "Ignoring\n";
		}
	}
}

void
usage()
{
	cerr << "Usage : ncpd [-s <socket number>] [-d <device>] [-b <baud rate>]\n";
	exit(1);
}

int
main(int argc, char **argv)
{
	ppsocket skt;
	IOWatch iow;
	skt.startup();

	// Command line parameter processing
	int sockNum = DPORT;
	int baudRate = DSPEED;
	const char *serialDevice = DDEV;
	short int nverbose = 0;
	short int pverbose = 0;
	short int lverbose = 0;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-s") && i + 1 < argc) {
			sockNum = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-d") && i + 1 < argc) {
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
		} else if (!strcmp(argv[i], "-b") && i + 1 < argc) {
			baudRate = atoi(argv[++i]);
		} else if (!strcmp(argv[i], "-V")) {
			cout << "plpnfsd version " << VERSION << endl;
			exit(0);
		} else
			usage();
	}

	if (!skt.listen("127.0.0.1", sockNum)) {
		cerr << "Could not initiate listen on socket " << sockNum << endl;
	} else {
		ncp *a = NULL;
		int numScp;
		socketChan *scp[8];

		while (true) {
			if (a == NULL) {
				a = new ncp(serialDevice, baudRate, iow);
				a->setVerbose(nverbose);
				a->setLinkVerbose(lverbose);
				a->setPktVerbose(pverbose);
				numScp = 0;
				iow.addIO(skt.socket());
			}
			// sockets
			pollSocketConnections(numScp, scp);
			checkForNewSocketConnection(skt, numScp, scp, a, iow);

			// psion
			a->poll();

			if (a->stuffToSend())
				iow.watch(0, 100000);
			else
				iow.watch(100000, 0);

			if (a->hasFailed()) {
				cout << "ncp: restarting\n";
				// resetSocketConnections(numScp, scp, a);
				// delete a;
				// a = NULL;
				a->reset();
			}
		}
		delete a;
	}
	skt.closeSocket();
}
