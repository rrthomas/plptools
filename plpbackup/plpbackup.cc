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
#include "config.h"
#endif

#include <sys/types.h>
#include <dirent.h>
#include <fstream.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>

#include "bool.h"
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
	*hlp << "Usage : plpbackup [-p <port>] [-v] [-f] [drive1:] [drive2:] ..." << endl;
	*hlp << endl;
	*hlp << "  Options:" << endl;
	*hlp << "    -h         Print this message and exit." << endl;
	*hlp << "    -V         Print version and exit." << endl;
	*hlp << "    -p <port>  Connect to ncpd using given port." << endl;
	*hlp << "    -v         Increase verbosity." << endl;
	*hlp << "    -f         Do a full backup. (Incremental otherwise)" << endl;
	*hlp << "    <drive>    A drive character. If none given, scan all drives" << endl;
}

bool full;
int verbose = 0;
bufferArray toBackup;
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
					long devbits;
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
	bufferArray files;
	char tmp[1024];

	strcpy(tmp, dir);
	strcat(tmp, "\\");
	if ((res = a->dir(tmp, files)) != rfsv::E_PSI_GEN_NONE)
		cerr << "Error: " << res << endl;
	else
		while (!files.empty()) {
			bufferStore s;

			s = files.pop();
			long size = s.getDWord(4);
			long attr = s.getDWord(8);
			strcpy(tmp, dir);
			strcat(tmp, "\\");
			strcat(tmp, s.getString(12));
			if (attr & rfsv::PSI_A_DIR) {
				collectFiles(a, tmp);
			} else {
				if ((attr & rfsv::PSI_A_ARCHIVE) || full) {
					s.truncate(12);
					s.addStringT(tmp);
					toBackup += s;
				}
			}
		}
}

static int
reportProgress(void *, long size)
{
	unsigned long percent;
	char pstr[10];
	char bstr[10];

	switch (verbose) {
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
	char dstPath[1024];
	struct passwd *pw;
	sigset_t sigset;

	sigemptyset(&sigset);
	sigaddset(&sigset, SIGPIPE);
	sigprocmask(SIG_BLOCK, &sigset, 0L);

	struct servent *se = getservbyname("psion", "tcp");
	endservent();
	if (se != 0L)
		sockNum = ntohs(se->s_port);

	// Command line parameter processing
	bool parmFound;
	do {
		parmFound = false;
		if ((argc > 1) && !strcmp(argv[1], "-V")) {
			cout << "plpbackup version " << VERSION << endl;
			exit(0);
		}
		if ((argc > 1) && !strcmp(argv[1], "-h")) {
			usage(&cout);
			exit(0);
		}
		if ((argc > 2) && !strcmp(argv[1], "-p")) {
			sockNum = atoi(argv[2]);
			argc -= 2;
			parmFound = true;
			for (int i = 1; i < argc; i++)
				argv[i] = argv[i + 2];
		}
		if ((argc > 1) && !strcmp(argv[1], "-v")) {
			verbose++;
			argc -= 1;
			parmFound = true;
			for (int i = 1; i < argc; i++)
				argv[i] = argv[i + 1];
		}
		if ((argc > 1) && !strcmp(argv[1], "-f")) {
			full = true;
			argc -= 1;
			parmFound = true;
			for (int i = 1; i < argc; i++)
				argv[i] = argv[i + 1];
		}
	} while (parmFound);

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
		if (verbose) {
			cout << "Performing " << (full ? "Full" : "Incremental") <<
				" backup to " << dstPath << endl;
			cout << "Stopping programs ..." << endl;
		}
		killsave(r, S5mx);
		if (argc > 1) {
			for (i = 1; i < argc; i++) {
				if ((strlen(argv[i]) != 2) || (argv[i][1] != ':')) {
					usage(&cerr);
					exit(1);
					runrestore(a, r);
				}
				if (verbose)
					cout << "Scanning Drive " << argv[i] << " ..." << endl;
				collectFiles(a, argv[i]);
			}
		} else {
			char drive[3];
			long devbits;
			long vtotal, vfree, vattr, vuniqueid;

			if (a->devlist(devbits) == rfsv::E_PSI_GEN_NONE) {
				for (i = 0; i < 26; i++) {
					if ((devbits & 1) && a->devinfo(i, vfree, vtotal, vattr, vuniqueid, NULL) == rfsv::E_PSI_GEN_NONE) {
						if (vattr != 7) {
							sprintf(drive, "%c:\0", 'A' + i);
							if (verbose)
								cout << "Scanning Drive " << drive << " ..." << endl;
							collectFiles(a, drive);
						}
					}
					devbits >>= 1;
				}
			} else
				cerr << "Couldn't get Drive list" << endl;
		}
		for (i = 0; i < toBackup.length(); i++) {
			bufferStore s = toBackup[i];
			backupSize += s.getDWord(4);
			backupCount++;
		}
		if (verbose)
			cout << "Size of backup: " << backupSize << " bytes in " <<
				backupCount << " files." << endl;
		if (backupCount == 0)
			cerr << "Nothing to backup" << endl;
		else {
			for (i = 0; i < toBackup.length(); i++) {
				bufferStore s = toBackup[i];
				const char *fn = s.getString(12);
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
				fileSize = s.getDWord(4);
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
				if (verbose)
					cout << "Writing index ..." << endl;
				strcpy(dest, dstPath);
				strcat(dest, ".index");
				ofstream op(dest);
				if (op) {
					op << "#plpbackup index" << endl;
					for (i = 0; i < toBackup.length(); i++) {
						bufferStore s = toBackup[i];
						PsiTime *t = (PsiTime *)s.getDWord(0);
						long size = s.getDWord(4);
						long attr = s.getDWord(8);
						const char *fn = s.getString(12);
						attr &= ~rfsv::PSI_A_ARCHIVE;
						op << hex
						   << setw(8) << setfill('0') <<
							t->getPsiTimeHi() << " "
						   << setw(8) << setfill('0') <<
							t->getPsiTimeLo() << " "
						   << setw(8) << setfill('0') <<
							size << " "
						   << setw(8) << setfill('0') <<
							attr << " "
						   << setw(0) << fn << endl;
					}
					op.close();
				} else {
					cerr << "Could not write index " << dest << endl;
					bErr = true;
				}
			}
			if (!bErr) {
				if (verbose)
					cout << "Resetting archive attributes ..." << endl;
					for (i = 0; i < toBackup.length(); i++) {
						bufferStore s = toBackup[i];
						long attr = s.getDWord(8);
						const char *fn = s.getString(12);
						if (attr & rfsv::PSI_A_ARCHIVE) {
							res = a->fsetattr(fn, 0,
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
		if (verbose)
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
