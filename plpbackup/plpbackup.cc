// $Id$
//
//  plpbackup - A backup program for Psion.
//
//  Copyright (C) 2000 Fritz Elfert <felfert@to.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <dirent.h>
#include <fstream.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <getopt.h>

#include "ppsocket.h"
#include "rfsv.h"
#include "rfsvfactory.h"
#include "rpcs.h"
#include "rpcsfactory.h"
#include "bufferstore.h"
#include "bufferarray.h"
#include "Enum.h"

void
usage(ostream *hlp)
{
	*hlp
		<< "Usage: plpbackup OPTIONS [<drive>:] [<drive>:] ..." << endl
		<< endl
		<< "  Options:" << endl
		<< "    -h, --help        Print this message and exit." << endl
		<< "    -V, --version     Print version and exit." << endl
		<< "    -p, --port=<port> Connect to ncpd using given port." << endl
		<< "    -v, --verbose     Increase verbosity." << endl
		<< "    -q, --quiet       Decrease verbosity." << endl
		<< "    -f, --full        Do a full backup (incremental otherwise)." << endl
		<< endl
		<< "  <drive> A drive character. If none given, scan all drives." << endl
		<< endl;
}

bool full;
int verbose = 0;
PlpDir toBackup;
unsigned long backupSize = 0;
unsigned long totalBytes = 0;
unsigned long fileSize = 0;
char home[1024];

static int
killsave(rpcs *r, bool S5mx) {
	Enum<rfsv::errs> res;
	bufferArray tmp;
	char psfile[1024];

	sprintf(psfile, "%s/.plpbackup.%d", home, getpid());
	if ((res = r->queryDrive('C', tmp)) != rfsv::E_PSI_GEN_NONE) {
		cerr << "Could not get process list, Error: " << res << endl;
		return 1;
	} else {
		ofstream op(psfile);
		if (!op) {
			cerr << "Could not write processlist " << psfile << endl;
			return 1;
		}
		op << "#plpbackup processlist" << endl;
		while (!tmp.empty()) {
			char pbuf[128];
			bufferStore cmdargs;
			bufferStore bs = tmp.pop();
			int pid = bs.getWord(0);
			const char *proc = bs.getString(2);
			if (S5mx)
				sprintf(pbuf, "%s.$%02d", proc, pid);
			else
				sprintf(pbuf, "%s.$%d", proc, pid);
			bs = tmp.pop();
			if (r->getCmdLine(pbuf, cmdargs) == 0)
				op << cmdargs.getString(0) << " " << bs.getString(0) << endl;
			if (verbose > 1)
				cout << cmdargs.getString(0) << " " << bs.getString(0) << endl;
			r->stopProgram(pbuf);
		}
		op.close();
	}
	return 0;
}

static int
runrestore(rfsv *a, rpcs *r) {
	Enum<rfsv::errs> res;
	bufferArray tmp;
	char psfile[1024];

	sprintf(psfile, "%s/.plpbackup.%d", home, getpid());
	ifstream ip(psfile);
	char cmd[512];
	char arg[512];

	if (!ip) {
		cerr << "Could not read processlist " << psfile << endl;
		return 1;
	}
	ip >> cmd >> arg;

	if (strcmp(cmd, "#plpbackup") || strcmp(arg, "processlist")) {
		ip.close();
		cerr << "Error: " << psfile <<
			" is not a process list saved with plpbackup" << endl;
		return 1;
	}
	while (!ip.eof()) {
		ip >> cmd >> arg;
		ip.get(&arg[strlen(arg)], sizeof(arg) - strlen(arg), '\n');
		if (strlen(cmd) > 0) {
			// Workaround for broken programs like Backlite. These do not store
			// the full program path. In that case we try running the arg1 which
			// results in starting the program via recog. facility.
			if (verbose > 1)
				cout << cmd << " " << arg << endl;
			if ((strlen(arg) > 2) && (arg[1] == ':') && (arg[0] >= 'A') &&
			    (arg[0] <= 'Z'))
				res = r->execProgram(arg, "");
			else
				res = r->execProgram(cmd, arg);
			if (res != rfsv::E_PSI_GEN_NONE) {
				// If we got an error here, that happened probably because
				// we have no path at all (e.g. Macro5) and the program is
				// not registered in the Psion's path properly. Now try
				// the ususal \System\Apps\<AppName>\<AppName>.app
				// on all drives.
				if (strchr(cmd, '\\') == NULL) {
					u_int32_t devbits;
					char tmp[512];
					if ((res = a->devlist(devbits)) == rfsv::E_PSI_GEN_NONE) {
						int i;
						for (i = 0; i < 26; i++) {
							if (devbits & 1) {
								sprintf(tmp,
									"%c:\\System\\Apps\\%s\\%s.app",
									'A' + i, cmd, cmd);
								res = r->execProgram(tmp, "");
							}
							if (res == rfsv::E_PSI_GEN_NONE)
								break;
						}
					}
				}
			}
			if (res != rfsv::E_PSI_GEN_NONE) {
				cerr << "Could not start " << cmd << " " << arg << endl;
				cerr << "Error: " << res << endl;
			}
		}
	}
	ip.close();
	unlink(psfile);
	return 0;
}

static void
collectFiles(rfsv *a, char *dir) {
	Enum<rfsv::errs> res;
	PlpDir files;
	char tmp[1024];

	strcpy(tmp, dir);
	strcat(tmp, "\\");
	if ((res = a->dir(tmp, files)) != rfsv::E_PSI_GEN_NONE)
		cerr << "Error: " << res << endl;
	else
		for (int i = 0; i < files.size(); i++) {
			PlpDirent e = files[i];

			// long size = s.getDWord(4);
			long attr = e.getAttr();
			strcpy(tmp, dir);
			strcat(tmp, "\\");
			strcat(tmp, e.getName());
			if (attr & rfsv::PSI_A_DIR) {
				collectFiles(a, tmp);
			} else {
				if ((attr & rfsv::PSI_A_ARCHIVE) || full) {
					e.setName(tmp);
					toBackup.push_back(e);
				}
			}
		}
}

static int
reportProgress(void *, u_int32_t size)
{
	unsigned long percent;
	char pstr[10];
	char bstr[10];

	switch (verbose) {
		case -1:
		case 0:
			return 1;
		case 1:
			percent = (totalBytes + size) * 100 / backupSize;
			break;
		case 2:
			percent = size * 100 / fileSize;
			break;
	}
	sprintf(pstr, " %3d%%", percent);
	memset(bstr, 8, sizeof(bstr));
	bstr[strlen(pstr)] = '\0';
	printf("%s%s", pstr, bstr);
	fflush(stdout);
	return 1;
}

int
mkdirp(char *path) {
	char *p = strchr(path, '/');
	while (p) {
		char csave = *(++p);
		*p = '\0';
		switch (mkdir(path, S_IRWXU|S_IRWXG)) {
			struct stat stbuf;

			case 0:
				break;
			default:
				if (errno != EEXIST) {
					perror(path);
					return 1;
				}
				stat(path, &stbuf);
				if (!S_ISDIR(stbuf.st_mode)) {
					perror(path);
					return 1;
				}
				break;
		}
		*p++ = csave;
		p = strchr(p, '/');
	}
	return 0;
}

static struct option opts[] = {
	{ "full",    no_argument,       0, 'f' },
	{ "help",    no_argument,       0, 'h' },
	{ "port",    required_argument, 0, 'V' },
	{ "verbose", no_argument,       0, 'v' },
	{ "quiet",   no_argument,       0, 'q' },
	{ "version", no_argument,       0, 'V' },
	{ 0,         0,                 0, 0   },
};

int
main(int argc, char **argv)
{
	ppsocket *skt;
	ppsocket *skt2;
	rfsv *a;
	rpcs *r;
	cpCallback_t cab = reportProgress;
	int status = 0;
	int sockNum = DPORT;
	int op;
	char dstPath[1024];
	struct passwd *pw;

	struct servent *se = getservbyname("psion", "tcp");
	endservent();
	if (se != 0L)
	  sockNum = ntohs(se->s_port);

	// Command line parameter processing
	opterr = 1;
	while ((op = getopt_long(argc, argv, "qfhp:vV", opts, NULL)) != EOF) {
		switch (op) {
			case 'V':
				cout << "plpbackup version " << VERSION << endl;
				exit(0);
			case 'h':
				usage(&cout);
				exit(0);
			case 'f':
				full = true;
				break;
			case 'v':
				verbose++;
				break;
			case 'q':
				verbose--;
				break;
			case 'p':
				sockNum = atoi(optarg);
				break;
			default:
				usage(&cerr);
				exit(1);
		}
	}
	for (int i = optind; i < argc; i++) {
		if ((strlen(argv[i]) != 2) ||
		    (toupper(argv[i][0]) < 'A') ||
		    (toupper(argv[i][0]) > 'Z') ||
		    (argv[i][1] != ':')) {
			usage(&cerr);
			exit(1);
		}
	}

	pw = getpwuid(getuid());
	if (pw && pw->pw_dir && strlen(pw->pw_dir)) {
		time_t now = time(0);
		char tstr[80];
		strcpy(home, pw->pw_dir);
		strftime(tstr, sizeof(tstr), "%Y-%m-%d-%H-%M-%S",
			 localtime(&now));
		sprintf(dstPath, "%s/plpbackup-%s/", home, tstr);
	} else {
		cerr << "Could not get user's home directory from /etc/passwd" << endl;
		exit(-1);
	}

	skt = new ppsocket();
	if (!skt->connect(NULL, sockNum)) {
		cerr << "plpbackup: could not connect to ncpd" << endl;
		return 1;
	}
	skt2 = new ppsocket();
	if (!skt2->connect(NULL, sockNum)) {
		cerr << "plpbackup: could not connect to ncpd" << endl;
		return 1;
	}
	rfsvfactory *rf = new rfsvfactory(skt);
	rpcsfactory *rp = new rpcsfactory(skt2);
	a = rf->create(false);
	r = rp->create(false);
	if ((a != NULL) && (r != NULL)) {
		Enum<rfsv::errs> res;
		Enum<rpcs::machs> machType;
		bool S5mx = false;
		bool bErr = false;
		int i;
		unsigned long backupCount = 0;
		char dest[1024];

		r->getMachineType(machType);
		if (machType == rpcs::PSI_MACH_S5) {
			rpcs::machineInfo mi;
			if ((res = r->getMachineInfo(mi)) == rfsv::E_PSI_GEN_NONE) {
				if (!strcmp(mi.machineName, "SERIES5mx"))
					S5mx = true;
			}
		}
		if (verbose >= 0) {
			cout << "Performing " << (full ? "full" : "incremental") <<
				" backup of ";
			if (optind < argc) {
				cout << "Drive ";
				for (i = optind; i < argc; ) {
					cout << argv[i++];
					if (i > optind) {
						if (i < (argc - 1))
							cout << ", ";
						else
							if (i < argc)
								cout << " and ";
					}
				}
			} else
				cout << "all drives";

			cout << " to " << dstPath << endl;
		}
		if (verbose > 0) {
			cout << "Stopping programs ..." << endl;
		}
		killsave(r, S5mx);
		if (optind < argc) {
			for (i = optind; i < argc; i++) {
				if (verbose > 0)
					cout << "Scanning Drive " << argv[i] << " ..." << endl;
				collectFiles(a, argv[i]);
			}
		} else {
			char drive[3];
			u_int32_t devbits;

			if (a->devlist(devbits) == rfsv::E_PSI_GEN_NONE) {
				for (i = 0; i < 26; i++) {
					PlpDrive psidr;
					if ((devbits & 1) && a->devinfo(i + 'A', psidr) == rfsv::E_PSI_GEN_NONE) {
						if (psidr.getMediaType() != 7) {
							sprintf(drive, "%c:\0", 'A' + i);
							if (verbose > 0)
								cout << "Scanning Drive " << drive << " ..." << endl;
							collectFiles(a, drive);
						}
					}
					devbits >>= 1;
				}
			} else
				cerr << "Couldn't get Drive list" << endl;
		}
		for (i = 0; i < toBackup.size(); i++) {
			backupSize += toBackup[i].getSize();
			backupCount++;
		}
		if (verbose > 0)
			cout << "Size of backup: " << backupSize << " bytes in " <<
				backupCount << " files." << endl;
		if (backupCount == 0)
			cerr << "Nothing to backup" << endl;
		else {
			for (i = 0; i < toBackup.size(); i++) {
				PlpDirent e = toBackup[i];
				const char *fn = e.getName();
				const char *p;
				char *q;
				char tmp[1024];

				for (p = fn, q = tmp; *p; p++, q++)
					switch (*p) {
						case '%':
							*q++ = '%';
							*q++ = '2';
							*q = '5';
							break;
						case '/':
							*q++ = '%';
							*q++ = '2';
							*q= 'f';
							break;
						case '\\':
							*q = '/';
							break;
						default:
							*q = *p;
					}
				*q = '\0';
				strcpy(dest, dstPath);
				strcat(dest, tmp);
				fileSize = e.getSize();
				if (verbose > 1)
					cout << "Backing up " << fn << flush;
				if (mkdirp(dest) != 0) {
					bErr = true;
					break;
				}
				res = a->copyFromPsion(fn, dest, NULL, cab);
				if (verbose > 1)
					cout << endl;
				totalBytes += fileSize;
				if (res != rfsv::E_PSI_GEN_NONE) {
					cerr << "Error during backup of " <<
						fn << ": " << res << endl;
					bErr = true;
					break;
				}
			}
			if (!bErr) {
				if (verbose > 0)
					cout << "Writing index ..." << endl;
				strcpy(dest, dstPath);
				strcat(dest, ".index");
				ofstream op(dest);
				if (op) {
					op << "#plpbackup index " <<
						(full ? "F" : "I") << endl;
					for (i = 0; i < toBackup.size(); i++) {
						PlpDirent e = toBackup[i];
						PsiTime t = e.getPsiTime();
						long attr = e.getAttr() &
							~rfsv::PSI_A_ARCHIVE;
						op << hex
						   << setw(8) << setfill('0') <<
							t.getPsiTimeHi() << " "
						   << setw(8) << setfill('0') <<
							t.getPsiTimeLo() << " "
						   << setw(8) << setfill('0') <<
							e.getSize() << " "
						   << setw(8) << setfill('0') <<
							attr << " "
						   << setw(0) << e.getName() << endl;
					}
					op.close();
				} else {
					cerr << "Could not write index " << dest << endl;
					bErr = true;
				}
			}
			if (!bErr) {
				if (verbose > 0)
					cout << "Resetting archive attributes ..." << endl;
				for (i = 0; i < toBackup.size(); i++) {
					PlpDirent e = toBackup[i];
					if (e.getAttr() & rfsv::PSI_A_ARCHIVE) {
						res = a->fsetattr(e.getName(), 0,
								  rfsv::PSI_A_ARCHIVE);
						if (res != rfsv::E_PSI_GEN_NONE) {
							bErr = true;
							break;
						}
					}
				}
			}
		}
		if (bErr)
			cerr << "Backup aborted due to error" << endl;
		if (verbose > 0)
			cout << "Restarting programs ..." << endl;
		runrestore(a, r);
		delete r;
		delete a;
	} else {
		if (!a)
			cerr << "plpbackup: could not create rfsv object" << endl;
		if (!r)
			cerr << "plpbackup: could not create rpcs object" << endl;
		exit(1);
	}
	return 0;
}
