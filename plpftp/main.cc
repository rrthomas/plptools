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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stream.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "bool.h"
#include "ppsocket.h"
#include "rfsv.h"
#include "rfsvfactory.h"
#include "rpcs.h"
#include "rpcsfactory.h"
#include "ftp.h"
#include "bufferstore.h"

void
usage()
{
	cout << "Version " << VERSION << endl;
	cout << "Usage : plpftp -p <port> [ftpcommand parameters]\n";
}

void
ftpHeader()
{
	cout << "PLPFTP Version " << VERSION;
	cout << " Copyright (C) 1999  Philip Proudman" << endl;
	cout << " Additions Copyright (C) 1999 Fritz Elfert <felfert@to.com>" << endl;
	cout << "                   & (C) 1999 Matt Gumbley <matt@gumbley.demon.co.uk>" << endl;
	cout << "PLP comes with ABSOLUTELY NO WARRANTY;" << endl;
	cout << "This is free software, and you are welcome to redistribute it" << endl;
	cout << "under GPL conditions; see the COPYING file in the distribution." << endl;
	cout << endl;
	cout << "FTP like interface started. Type \"?\" for help." << endl;
}

int
main(int argc, char **argv)
{
	ppsocket *skt;
	ppsocket *skt2;
	rfsv *a;
	rpcs *r;
	ftp f;
	int status = 0;
	sigset_t sigset;

	// Command line parameter processing
	int sockNum = DPORT;
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGPIPE);
	sigprocmask(SIG_BLOCK, &sigset, 0L);

	if ((argc > 2) && !strcmp(argv[1], "-p")) {
		sockNum = atoi(argv[2]);
		argc -= 2;
		for (int i=1; i<argc; i++)
			argv[i] = argv[i+2];
	}

	if (argc < 2)
		ftpHeader();
	skt = new ppsocket();
	if (!skt->connect(NULL, sockNum)) {
		cout << "plpftp: could not connect to ncpd" << endl;
		return 1;
	}
	skt2 = new ppsocket();
	if (!skt2->connect(NULL, sockNum)) {
		cout << "plpftp: could not connect to ncpd" << endl;
		return 1;
	}
	rfsvfactory *rf = new rfsvfactory(skt);
	rpcsfactory *rp = new rpcsfactory(skt2);
	a = rf->create(false);
	r = rp->create(false);
	if ((a != NULL) && (r != NULL)) {
		status = f.session(*a, *r, argc, argv);
		delete r;
		delete a;
	} else {
		cout << "plpftp: could not create rfsv object" << endl;
		status = 1;
	}
	return status;
}
