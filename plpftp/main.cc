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
#include <config.h>
#endif

#include <stream.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <plpintl.h>
#include <ppsocket.h>
#include <rfsv.h>
#include <rfsvfactory.h>
#include <rpcs.h>
#include <rpcsfactory.h>
#include <bufferstore.h>

#include "ftp.h"

void
usage()
{
    cout << _("Version ") << VERSION << endl;
    cout << _("Usage : plpftp -p <port> [ftpcommand parameters]") << endl;
}

void
ftpHeader()
{
    cout << _("PLPFTP Version ") << VERSION;
    cout << _(" Copyright (C) 1999  Philip Proudman") << endl;
    cout << _(" Additions Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>") << endl;
    cout << _("                   & (C) 1999 Matt Gumbley <matt@gumbley.demon.co.uk>") << endl;
    cout << _("PLPFTP comes with ABSOLUTELY NO WARRANTY.") << endl;
    cout << _("This is free software, and you are welcome to redistribute it") << endl;
    cout << _("under GPL conditions; see the COPYING file in the distribution.") << endl;
    cout << endl;
    cout << _("FTP like interface started. Type \"?\" for help.") << endl;
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
    int sockNum = DPORT;

    setlocale (LC_ALL, "");
    textdomain(PACKAGE);

    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != 0L)
	sockNum = ntohs(se->s_port);

    // Command line parameter processing
    if ((argc > 2) && !strcmp(argv[1], "-p")) {
	sockNum = atoi(argv[2]);
	argc -= 2;
	for (int i = 1; i < argc; i++)
	    argv[i] = argv[i + 2];
    }

    if (argc < 2)
	ftpHeader();
    skt = new ppsocket();
    if (!skt->connect(NULL, sockNum)) {
	cout << _("plpftp: could not connect to ncpd") << endl;
	return 1;
    }
    skt2 = new ppsocket();
    if (!skt2->connect(NULL, sockNum)) {
	cout << _("plpftp: could not connect to ncpd") << endl;
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
	delete skt;
	delete skt2;
    } else {
	cerr << "plpftp: " << X_(rf->getError()) << endl;
	status = 1;
    }
    delete rf;
    delete rp;
    return status;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
