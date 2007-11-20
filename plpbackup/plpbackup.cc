/* -*- Mode: C++ -*-
 * $Id$
 *
 * This file is part of plptools.
 *
 * Copyright (C) 2000-2001 Fritz Elfert <felfert@to.com>
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

#include <plpintl.h>
#include <ppsocket.h>
#include <rfsv.h>
#include <rfsvfactory.h>
#include <rpcs.h>
#include <rpcsfactory.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <getopt.h>

using namespace std;

bool doStart = false;
bool doStop = false;
bool doAbort = false;

int verbose = 0;
int caughtSig = 0;

rfsv *Rfsv;
rpcs *Rpcs;

static RETSIGTYPE
sig_handler(int sig)
{
    caughtSig = sig;
    signal(sig, sig_handler);
}

static bool
checkAbort()
{
    if (caughtSig) {
	if (verbose > 0)
	    cerr << endl;
	cerr << _("Got signal ");
	switch (caughtSig) {
        case SIGTERM:
            cerr << "SIGTERM";
            break;
        case SIGINT:
            cerr << "SIGINT";
            break;
        default:
            cerr << caughtSig;
	}
	cerr << endl;
	cerr << _("Abort? (y/N) ") << flush;
	const char *validA = _("Y");
	char answer;
	cin >> answer;
	caughtSig = 0;
	if (toupper(answer) == *validA)
	    doAbort = true;
    }
    return doAbort;
}

static int
stopPrograms(const char *file) {
    Enum<rfsv::errs> res;
    processList tmp;
    FILE *fp = fopen(file, "w");

    if (fp == NULL) {
        cerr << _("Could not open command list file ") << file << endl;
        return 1;
    }
    if (verbose > 0)
	cerr << _("Stopping programs, writing list to ...") << file << endl;
    if ((res = Rpcs->queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE) {
	cerr << _("plpbackup: Could not get process list: ") << res << endl;
	return 1;
    } else {
	for (processList::iterator i = tmp.begin(); i != tmp.end(); i++) {
	    fputs(i->getArgs(), fp);
            fputc('\n', fp);
	    Rpcs->stopProgram(i->getProcId());
	}
	time_t tstart = time(0) + 5;
	while (true) {
	    usleep(100000);
	    if (checkAbort())
		return 1;
	    if ((res = Rpcs->queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE) {
		cerr << "Could not get process list, Error: " << res << endl;
		return 1;
	    }
	    if (tmp.empty())
		break;
	    if (time(0) > tstart) {
		cerr << _(
                          "Could not stop all processes. Please stop running\n"
                          "programs manually on the Psion, then hit return.") << flush;
		cin.getline((char *)&tstart, 1);
		tstart = time(0) + 5;
	    }
	}
    }
    fclose(fp);
    return 0;
}

static char *
getln(FILE *fp)
{
    size_t len = 256;
    int c;
    char *l = (char *)malloc(len), *s = l;
    
    assert(l);
    for (c = getc(fp); c != '\n' && c != EOF; c = getc(fp)) {
        if (s == l + len) {
            l = (char *)realloc(l, len * 2);
            assert(l);
            len *= 2;
        }
        *s++ = c;
    }
    if (s == l + len) {
        l = (char *)realloc(l, len + 1);
        assert(l);
    }
    *s++ = '\0';
    
    l = (char *)realloc(l, s - l);
    assert(l);
    return l;
}

static int
startPrograms(const char *file) {
    Enum<rfsv::errs> res;
    FILE *fp = fopen(file, "r");
    string cmd;

    if (fp == NULL) {
        cerr << _("Could not open command list file ") << file << endl;
        return 1;
    }
    if (verbose > 0)
	cerr << _("Restarting programs, reading list from ") << file << endl;
    for (cmd = string(getln(fp)); cmd.length() > 0; cmd = string(getln(fp))) {
	int firstBlank = cmd.find(' ');
	string prog = string(cmd, 0, firstBlank);
	string arg = string(cmd, firstBlank + 1);

	if (!prog.empty()) {
	    if (verbose > 1)
		cerr << cmd << endl;

	    // Workaround for broken programs like Backlite. These do not store
	    // the full program path. In that case we try running the arg1 which
	    // results in starting the program via recog. facility.
	    if ((arg.size() > 2) && (arg[1] == ':') && (arg[0] >= 'A') &&
		(arg[0] <= 'Z'))
		res = Rpcs->execProgram(arg.c_str(), "");
	    else
		res = Rpcs->execProgram(prog.c_str(), arg.c_str());
	    if (res != rfsv::E_PSI_GEN_NONE) {
		// If we got an error here, that happened probably because
		// we have no path at all (e.g. Macro5) and the program is
		// not registered in the Psion's path properly. Now try
		// the usual \System\Apps\<AppName>\<AppName>.app
		// on all drives.
		if (prog.find('\\') == prog.npos) {
		    u_int32_t devbits;
		    if ((res = Rfsv->devlist(devbits)) == rfsv::E_PSI_GEN_NONE) {
			int i;
			for (i = 0; i < 26; i++) {
			    if (devbits & (1 << i)) {
                                string tmp;
				tmp = (char)('A' + i) + ":\\System\\Apps\\" +
                                    prog + "\\" + prog + ".app";
				res = Rpcs->execProgram(tmp.c_str(), "");
                                if (res == rfsv::E_PSI_GEN_NONE)
                                    break;
			    }
			}
		    }
		}
	    }
	    if (res != rfsv::E_PSI_GEN_NONE) {
		cerr << "Could not start " << cmd << endl;
		cerr << "Error: " << res << endl;
	    }
	}
    }
    return 0;
}

void
usage(void)
{
    cerr <<
	_("Usage: plpbackup OPTION...\n"
	  "\n"
	  "  Options:\n"
	  "    -h, --help             Print this message and exit.\n"
	  "    -V, --version          Print version and exit.\n"
	  "    -p, --port=[HOST:]PORT Connect to ncpd on HOST, PORT.\n"
	  "    -v, --verbose          Increase verbosity.\n"
	  "    -q, --quiet            Decrease verbosity.\n"
	  "    --stop=FILE            Stop programs and write list to FILE.\n"
	  "    --start=FILE           Start programs from list in FILE.\n"
	  "\n");
    exit(0);
}

static struct option opts[] = {
    { "help",    no_argument,       0, 'h' },
    { "port",    required_argument, 0, 'p' },
    { "verbose", no_argument,       0, 'v' },
    { "quiet",   no_argument,       0, 'q' },
    { "stop",    required_argument, 0, 1   },
    { "start",   required_argument, 0, 2   },
    { "version", no_argument,       0, 'v' },
    { 0,         0,                 0, 0   },
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

int
main(int argc, char **argv)
{
    ppsocket *skt;
    ppsocket *skt2;
    const char *host = "127.0.0.1";
    char *file = NULL;
    int sockNum = DPORT;
    int op;

#ifdef LC_ALL
    setlocale (LC_ALL, "");
#endif
    textdomain(PACKAGE);

    struct servent *se = getservbyname("psion", "tcp");
    endservent();
    if (se != 0L)
	sockNum = ntohs(se->s_port);

    // Command line parameter processing
    opterr = 1;
    while ((op = getopt_long(argc, argv, "fFhqvVp:", opts, NULL)) != -1) {
	switch (op) {
        case 1:
            doStop = true;
            file = strdup(optarg);
            break;
        case 2:
            doStart = true;
            file = strdup(optarg);
            break;
        case 'V':
            cerr << _("plpbackup version ") << VERSION << endl;
            exit(0);
        case 'h':
            usage();
            break;
        case 'v':
            verbose++;
            break;
        case 'q':
            verbose--;
            break;
        case 'p':
            parse_destination(optarg, &host, &sockNum);
            break;
        default:
            usage();
	}
    }
    if (doStop && doStart) {
	cerr << _("Cannot both start and stop.")
	     << endl;
	usage();
    }
    if (!(doStop || doStart)) {
	cerr << _("No action specified.") << endl;
	usage();
    }

    signal(SIGTERM, sig_handler);
    signal(SIGINT, sig_handler);
    // Connect to Psion
    skt = new ppsocket();
    if (!skt->connect(host, sockNum)) {
	cerr << _("plpbackup: could not connect to ncpd") << endl;
	return 1;
    }
    skt2 = new ppsocket();
    if (!skt2->connect(host, sockNum)) {
	cerr << _("plpbackup: could not connect to ncpd") << endl;
	return 1;
    }
    rfsvfactory *rf = new rfsvfactory(skt);
    rpcsfactory *rp = new rpcsfactory(skt2);
    Rfsv = rf->create(false);
    if (Rfsv == NULL) {
	cerr << "plpbackup: " << X_(rf->getError()) << endl;
	exit(1);
    }
    Rpcs = rp->create(false);
    if (Rpcs == NULL) {
	cerr << "plpbackup: " << X_(rp->getError()) << endl;
	exit(1);
    }

    Enum<rfsv::errs> res;
    Enum<rpcs::machs> machType;

    Rpcs->getMachineType(machType);
    if (doStop)
	stopPrograms(file);
    if (doStart)
	startPrograms(file);
    delete Rpcs;
    delete Rfsv;
    return 0;
}
