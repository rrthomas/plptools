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

#include <stream.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "defs.h"
#include "bool.h"
#include "ppsocket.h"
#include "rfsv32.h"
#include "ftp.h"
#include "bufferstore.h"

void
usage()
{
	cout << "Version " << VERSION << endl;
	cout << "Usage : plpftp -s <socket number> <command> <parameters>\n";
}

void
ftpHeader()
{
	cout << "PLPFTP Version " << VERSION;
	cout << " Copyright (C) 1999  Philip Proudman" << endl;
	cout << " Additions Copyright (C) 1999 Fritz Elfert <felfert@to.com>" << endl;
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
	bool res;

	// Command line parameter processing
	int sockNum = DPORT;

	if (!strcmp(argv[1], "-s") && (argc > 2)) {
		sockNum = atoi(argv[2]);
		argc -= 2;
		for (int i=1; i < argc; i++)
			argv[i] = argv[i+2];
	}

	skt = new ppsocket();
	skt->startup();
	res = skt->connect(NULL, sockNum);
	if (!res) {
		delete skt;

		// Let's try to start a daemon
		char temp[200];
		sprintf(temp, "ncp -s %d > /dev/null &", sockNum);
		system(temp);

		for (int retry = 0; !res && retry < 40; retry++) {
			usleep(100000);
			skt = new ppsocket();
			skt->startup();
			res = skt->connect(NULL, sockNum);
			if (!res)
				delete skt;
		}

		if (!res) {
			delete skt;
			cout << "Can't connect to ncp daemon on socket " << sockNum << endl;
			return 1;
		}
		sleep(2);	// Let the psion connect

	}
	rfsv32 *a;
	while (true) {
		a = new rfsv32(skt);
		if (a->getStatus() == 0)
			break;
		delete a;
		delete skt;
		cerr << "No Link available, waiting 2 secs ..." << endl;
		sleep(2);
		skt = new ppsocket();
		skt->startup();
		skt->connect(NULL, sockNum);
	}

	int status = 0;
	ftp f;
	status = f.session(*a, argc, argv);

	if (status != 0)
		cerr << "Command failed\n";
	delete a;
	return 0;
}
