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

#include <sys/types.h>
#include <dirent.h>
#include <fstream>
#include <strstream>
#include <iomanip>
#include <vector>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <errno.h>
#include <pwd.h>
#include <getopt.h>

#include <plpintl.h>
#include <ppsocket.h>
#include <rfsv.h>
#include <rfsvfactory.h>
#include <rpcs.h>
#include <rpcsfactory.h>
#include <bufferstore.h>
#include <bufferarray.h>

bool full = false;
bool S5mx = false;
bool doRestore = false;
bool doBackup = false;
bool doFormat = false;
bool skipError = false;
bool doAbort = false;

int    verbose = 0;
int    caughtSig = 0;
unsigned long backupSize = 0;
unsigned long backupCount = 0;
unsigned long totalBytes = 0;
unsigned long fileSize = 0;

PlpDir toBackup;
vector<string> driveList;
vector<string> archList;
vector<string> savedCommands;
rfsv *Rfsv;
rpcs *Rpcs;

static RETSIGTYPE
sig_handler(int sig)
{
	caughtSig = sig;
	signal(sig, sig_handler);
};

static const char * const
getHomeDir()
{
    struct passwd *pw = getpwuid(getuid());
    if (pw && pw->pw_dir && strlen(pw->pw_dir))
	return pw->pw_dir;
    else
	cerr << _("Could not get user's home directory from /etc/passwd")
	     << endl;
    return "";
}

static const char * const
generateBackupName()
{
    time_t now = time(0);
    Enum<rfsv::errs> res;
    char tstr[80];
    static char nbuf[4096];

    rpcs::machineInfo mi;
    if ((res = Rpcs->getMachineInfo(mi)) != rfsv::E_PSI_GEN_NONE) {
	cerr << _("Could not get machine UID") << endl;
	exit(1);
    }
    strftime(tstr, sizeof(tstr), "-%Y-%m-%d-%H-%M-%S", localtime(&now));
    sprintf(nbuf, "%s/plpbackup/%16llx/%c%s.tar.gz", getHomeDir(),
	    mi.machineUID, full ? 'F' : 'I', tstr);
    return nbuf;
}

static const char * const
generateTmpDir()
{
    char *tmpdir;
    static char nbuf[4096];

    if (!(tmpdir = getenv("TMPDIR")))
	tmpdir = P_tmpdir;
    sprintf(nbuf, "%s/plpbackup_%d", tmpdir, getpid());
    mkdir(nbuf, S_IRWXU|S_IRGRP|S_IXGRP);
    return nbuf;
}

static bool
checkAbort()
{
    if (caughtSig) {
	if (verbose > 0)
	    cout << endl;
	cout << _("Got signal ");
	switch (caughtSig) {
	    case SIGTERM:
		cout << "SIGTERM";
		break;
	    case SIGINT:
		cout << "SIGINT";
		break;
	    default:
		cout << caughtSig;
	}
	cout << endl;
	cout << _("Abort? (y/N) ") << flush;
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
stopPrograms() {
    Enum<rfsv::errs> res;
    bufferArray tmp;

    if (verbose > 0)
	cout << _("Stopping programs ...") << endl;
    if ((res = Rpcs->queryDrive('C', tmp)) != rfsv::E_PSI_GEN_NONE) {
	cerr << _("plpbackup: Could not get process list: ") << res << endl;
	return 1;
    } else {
	while (!tmp.empty()) {
	    ostrstream pbuf;
	    bufferStore cmdargs;
	    bufferStore bs = tmp.pop();
	    int pid = bs.getWord(0);

	    pbuf << bs.getString(2) << ".$";
	    if (S5mx)
		pbuf << dec << setw(2) << setfill(0) << pid;
	    else
		pbuf << dec << pid;
	    pbuf << '\0';
	    bs = tmp.pop();
	    if (Rpcs->getCmdLine(pbuf.str(), cmdargs) == 0) {
		string cmdline = cmdargs.getString(0);
		cmdline += " ";
		cmdline += bs.getString(0);
		savedCommands.push_back(cmdline);
		if (verbose > 1)
		    cout << cmdline << endl;
	    }
	    Rpcs->stopProgram(pbuf.str());
	}
	time_t tstart = time(0) + 5;
	while (true) {
	    usleep(100000);
	    if (checkAbort())
		return 1;
	    if ((res = Rpcs->queryDrive('C', tmp)) != rfsv::E_PSI_GEN_NONE) {
		cerr << "Could not get process list, Error: " << res << endl;
		return 1;
	    }
	    if (tmp.empty())
		break;
	    if (time(0) > tstart) {
		cout << _(
		    "Could not stop all processes. Please stop running\n"
		    "programs manually on the Psion, then hit return.") << flush;
		cin.getline((char *)&tstart, 1);
		tstart = time(0) + 5;
	    }
	}
    }
    return 0;
}

static int
startPrograms() {
    Enum<rfsv::errs> res;

    if (verbose > 0)
	cout << _("Restarting programs ...") << endl;
    for (int i = 0; i < savedCommands.size(); i++) {
	int firstBlank = savedCommands[i].find(' ');
	string cmd = string(savedCommands[i], 0, firstBlank);
	string arg = string(savedCommands[i], firstBlank + 1);

	if (!cmd.empty()) {
	    if (verbose > 1)
		cout << cmd << " " << arg << endl;

	    // Workaround for broken programs like Backlite. These do not store
	    // the full program path. In that case we try running the arg1 which
	    // results in starting the program via recog. facility.
	    if ((arg.size() > 2) && (arg[1] == ':') && (arg[0] >= 'A') &&
		(arg[0] <= 'Z'))
		res = Rpcs->execProgram(arg.c_str(), "");
	    else
		res = Rpcs->execProgram(cmd.c_str(), arg.c_str());
	    if (res != rfsv::E_PSI_GEN_NONE) {
		// If we got an error here, that happened probably because
		// we have no path at all (e.g. Macro5) and the program is
		// not registered in the Psion's path properly. Now try
		// the ususal \System\Apps\<AppName>\<AppName>.app
		// on all drives.
		if (cmd.find('\\') == -1) {
		    u_int32_t devbits;
		    if ((res = Rfsv->devlist(devbits)) == rfsv::E_PSI_GEN_NONE) {
			int i;
			for (i = 0; i < 26; i++) {
			    if (devbits & 1) {
				ostrstream tmp;
				tmp << 'A' + i << "\\System\\Apps\\"
				    << cmd << "\\" << cmd << ".app";
				res = Rpcs->execProgram(tmp.str(), "");
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
    return 0;
}

string
unix2psion(const char * const path) {
    string tmp = path;
    int pos;

    while ((pos = tmp.find('/')) != -1)
	tmp.replace(pos, 1, "\\");
    while ((pos = tmp.find("%2f")) != -1)
	tmp.replace(pos, 3, "/");
    while ((pos = tmp.find("%25")) != -1)
	tmp.replace(pos, 3, "%");
    return tmp;
}

string
psion2unix(const char * const path) {
    string tmp;

    for (const char *p = path; p && *p; p++)
	switch (*p) {
	    case '%':
		tmp += "%25";
		break;
	    case '/':
		tmp += "%2f";
		break;
	    case '\\':
		tmp += "/";
		break;
	    default:
		tmp += *p;
	}
    return tmp;
}

static void
collectFiles(bool &found, const char *dir) {
    Enum<rfsv::errs> res;
    PlpDir files;
    string tmp;

    tmp = dir;
    tmp += "\\";
    if ((res = Rfsv->dir(tmp.c_str(), files)) != rfsv::E_PSI_GEN_NONE)
	cerr << "Error: " << res << endl;
    else
	for (int i = 0; i < files.size(); i++) {
	    PlpDirent e = files[i];

	    if (checkAbort())
		return;
	    // long size = s.getDWord(4);
	    long attr = e.getAttr();
	    tmp = dir;
	    tmp += "\\";
	    tmp += e.getName();
	    if (attr & rfsv::PSI_A_DIR) {
		collectFiles(found, tmp.c_str());
	    } else {
		if ((attr & rfsv::PSI_A_ARCHIVE) || full) {
		    e.setName(tmp.c_str());
		    toBackup.push_back(e);
		    found |= true;
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

    if (checkAbort())
	return 0;
    if (verbose < 1)
	return 1;
    switch (verbose) {
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

static cpCallback_t cab = reportProgress;

static int
mkdirp(const char *_path) {
    char *path = strdup(_path);
    char *p = strchr(path, '/');

    while (p) {
	char csave = *(++p);
	*p = '\0';
	switch (mkdir(path,  S_IRWXU|S_IRGRP|S_IXGRP)) {
	    struct stat stbuf;

	    case 0:
		break;
	    default:
		if (errno != EEXIST) {
		    perror(path);
		    free(path);
		    return 1;
		}
		stat(path, &stbuf);
		if (!S_ISDIR(stbuf.st_mode)) {
		    perror(path);
		    free(path);
		    return 1;
		}
		break;
	}
	*p++ = csave;
	p = strchr(p, '/');
    }
    free(path);
    return 0;
}

static void
rmrf(const char *_path)
{
    DIR *d = opendir(_path);
    if (d) {
	struct dirent *de;
	while ((de = readdir(d))) {
	    struct stat st;
	    if ((de->d_name[0] == '.') &&
		((de->d_name[1] == '\0') ||
		 ((de->d_name[1] == '.') &&
		  (de->d_name[2] == '\0'))))
		continue;
	    string path = _path;
	    path += "/";
	    path += de->d_name;
	    if (stat(path.c_str(), &st) == 0) {
		if (S_ISDIR(st.st_mode))
		    rmrf(path.c_str());
		else
		    unlink(path.c_str());
	    }
	}
    }
    closedir(d);
    rmdir(_path);
}

static void
startMessage(const char *arch)
{
    if (verbose >= 0) {
	cout << "Performing ";
	if (doFormat)
	    cout << "format";
	if (doRestore) {
	    if (doFormat)
		cout << " and ";
	    cout << "restore";
	} else
	    cout << (full ? "full" : "incremental") << " backup";
	cout << " of ";
	if (driveList.empty())
	    cout << "all drives";
	else {
	    cout << "Drive ";
	    for (int i = 0; i < driveList.size(); i++) {
		cout << driveList[i++];
		if (i < (driveList.size() - 1))
		    cout << ", ";
		else {
		    if (driveList.size() > 1)
			cout << " and ";
		}
	    }
	}
	if (arch) {
	    if (doBackup)
		cout << " to " << arch;
	    if (doRestore)
		cout << " from " << arch;
	}
	cout << endl;
    }

}

static double
tdiff(struct timeval *start, struct timeval *end)
{
    double s = (double)start->tv_sec * 1000000.0;
    double e = (double)end->tv_sec * 1000000.0;
    s += start->tv_usec;
    e += end->tv_usec;
    return (e - s) / 1000000.0;
}

static int
runFormat()
{
    return 0;
}

static void
runRestore()
{
    int i;
    ostrstream tarcmd;
    for (i = 0; i < archList.size(); i++) {
	tarcmd << "tar --to-stdout -xzf " << archList[i]
	       << " 'KPsion*Index'" << '\0';
	FILE *f = popen(tarcmd.str(), "r");

	pclose(f);
    }
}

static void
runBackup()
{
    vector<char*>backupDrives;
    Enum<rfsv::errs> res;
    string dstPath;
    string archPath;
    bool bErr = false;
    bool found;
    int i;
    struct timeval start_tv, end_tv, cstart_tv, cend_tv, sstart_tv, send_tv;

    gettimeofday(&start_tv, NULL);
    if (archList.empty())
	archPath = generateBackupName();
    else
	archPath = archList[0];
    dstPath = generateTmpDir();

    startMessage(archPath.c_str());

    // Stop all programs on Psion
    stopPrograms();
    if (checkAbort()) {
	// remove temporary dir
	rmrf(dstPath.c_str());
	// restart previously killed programs
	startPrograms();
	cout << _("Backup aborted by user") << endl;
	return;
    }

    gettimeofday(&sstart_tv, NULL);
    // Scan for files to be backed up
    backupCount = 0;
    if (driveList.empty()) {
	char drive[3];
	u_int32_t devbits;

	if (Rfsv->devlist(devbits) == rfsv::E_PSI_GEN_NONE) {
	    for (i = 0; i < 26; i++) {
		PlpDrive psidr;
		if ((devbits & 1) && Rfsv->devinfo(i + 'A', psidr)
		    == rfsv::E_PSI_GEN_NONE) {
		    if (psidr.getMediaType() != 7) {
			sprintf(drive, "%c:\0", 'A' + i);
			if (verbose > 0)
			    cout << _("Scanning Drive ") << drive << " ..."
				 << flush;
			found = false;
			collectFiles(found, drive);
			if (verbose > 0)
			    cout << endl;
			if (found)
			    backupDrives.push_back(drive);
		    }
		}
		devbits >>= 1;
	    }
	} else
	    cerr << _("plpbackup: Couldn't get Drive list") << endl;
    } else {
	for (i = 0; i < driveList.size(); i++) {
	    char *drive = (char *)driveList[i].c_str();
	    if (verbose > 0)
		cout << _("Scanning Drive ") << drive << " ..." << flush;
	    found = false;
	    collectFiles(found, drive);
	    if (verbose > 0)
		cout << endl;
	    if (found)
		backupDrives.push_back(drive);
	}
    }
    gettimeofday(&send_tv, NULL);
    if (checkAbort()) {
	// remove temporary dir
	rmrf(dstPath.c_str());
	// restart previously killed programs
	startPrograms();
	cout << _("Backup aborted by user") << endl;
	return;
    }

    // Calculate number of files and total bytecount
    for (i = 0; i < toBackup.size(); i++) {
	backupSize += toBackup[i].getSize();
	backupCount++;
    }
    if (verbose > 0)
	cout << _("Size of backup: ") << backupSize << _(" bytes in ") <<
	    backupCount << _(" files.") << endl;

    if (backupCount == 0)
	cerr << _("Nothing to backup") << endl;
    else {
	string dest;

	gettimeofday(&cstart_tv, NULL);
	// copy all files to local temporary dir
	for (i = 0; i < toBackup.size(); i++) {
	    struct timeval fstart_tv, fend_tv;
	    PlpDirent e = toBackup[i];
	    const char *fn = e.getName();

	    dest = dstPath;
	    dest += '/';
	    dest += psion2unix(fn);
	    fileSize = e.getSize();
	    if (verbose > 1)
		cout << _("Backing up ") << fn << flush;
	    if (mkdirp(dest.c_str()) != 0) {
		bErr = true;
		break;
	    }
	    gettimeofday(&fstart_tv, NULL);
	    res = Rfsv->copyFromPsion(fn, dest.c_str(), NULL, cab);
	    if (checkAbort()) {
		// remove temporary dir
		rmrf(dstPath.c_str());
		// restart previously killed programs
		startPrograms();
		cout << _("Backup aborted by user") << endl;
		return;
	    }
	    gettimeofday(&fend_tv, NULL);
	    if (verbose > 1)
		cout << " "
		     << (int)((double)fileSize / tdiff(&fstart_tv, &fend_tv))
		     << " CPS" << endl;
	    totalBytes += fileSize;
	    if (res != rfsv::E_PSI_GEN_NONE) {
		if (skipError) {
		    e.setName("!");
		    if (verbose > 0)
			cerr << _("Skipping ") << fn << ": "
			     << res << endl;
		} else {
		    cerr << _("Error during backup of ") << fn << ": "
			 << res << endl;
		    if (isatty(0)) {
			bool askLoop = true;
			do {
			    char answer;
			    string vans = _("STAR");

			    cerr << _("(S)kip all, Skip (t)his, (A)bort, (R)etry: ")
				 << flush;
			    cin >> answer;
			    switch (vans.find(toupper(answer))) {
				case 0:
				    skipError = true;
				    // fall thru
				case 1:
				    e.setName("!");
				    askLoop = false;
				    break;
				case 2:
				    bErr = true;
				    i = toBackup.size();
				    askLoop = false;
				    break;
				case 3:
				    if (verbose > 1)
					cout << _("Backing up ") << fn << flush;
				    break;
				    res = Rfsv->copyFromPsion(fn, dest.c_str(), NULL, cab);
				    if (checkAbort()) {
					// remove temporary dir
					rmrf(dstPath.c_str());
					// restart previously killed programs
					startPrograms();
					cout << _("Backup aborted by user")
					     << endl;
					return;
				    }
				    if (verbose > 1)
					cout << endl;
				    if (res != rfsv::E_PSI_GEN_NONE) {
					cerr << _("Error during backup of ")
					     << fn << ": " << res << endl;
				    } else
					askLoop = false;
				    break;
			    }
			} while (askLoop);
		    } else {
			bErr = true;
			break;
		    }
		}
	    }
	}
	gettimeofday(&cend_tv, NULL);

	// Create index file
	if (!bErr) {
	    if (verbose > 0)
		cout << _("Writing index ...") << endl;
	    dest = dstPath;
	    dest += "/KPsion";
	    dest += ((full) ? "Full" : "Incremental");
	    dest += "Index";
	    ofstream op(dest.c_str());
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
		cerr << _("plpbackup: Could not write index ") << dest << endl;
		bErr = true;
	    }
	}

	// tar it all up
	if (!bErr) {
	    ostrstream tarcmd;

	    if (verbose > 0)
		cout << _("Creating tar archive ...") << endl;

	    tarcmd << "tar czCf";
	    if (verbose > 1)
		tarcmd << 'v';
	    tarcmd << " " << dstPath << " " << archPath << " KPsion";
	    tarcmd << (full ? "Full" : "Incremental") << "Index";
	    for (i = 0; i < backupDrives.size(); i++)
		tarcmd << " " << backupDrives[i];
	    tarcmd << '\0';

	    mkdirp(archPath.c_str());
	    if (system(tarcmd.str())) {
		cerr << _("plpbackup: Error during execution of ")
		     << tarcmd.str() << endl;
		unlink(archPath.c_str());
		bErr = true;
	    }
	}

	// finally reset archive attributes
	if (!bErr) {
	    if (verbose > 0)
		cout << _("Resetting archive attributes ...") << endl;
	    for (i = 0; i < toBackup.size(); i++) {
		PlpDirent e = toBackup[i];
		if (e.getAttr() & rfsv::PSI_A_ARCHIVE) {
		    res = Rfsv->fsetattr(e.getName(), 0,
				      rfsv::PSI_A_ARCHIVE);
		    if (res != rfsv::E_PSI_GEN_NONE) {
			bErr = true;
			break;
		    }
		}
	    }
	}
    }

    // remove temporary dir
    rmrf(dstPath.c_str());

    if (bErr)
	cerr << _("Backup aborted due to error") << endl;

    // restart previously killed programs
    startPrograms();
    gettimeofday(&end_tv, NULL);
    if (!checkAbort() && verbose > 0) {
	cout << _("Total time elapsed:     ") << tdiff(&start_tv, &end_tv)
	     << endl;
	cout << _("Time for scanning:      ") << tdiff(&sstart_tv, &send_tv)
	     << endl;
	if (backupSize > 0) {
	    cout << _("Time for transfer:      ") << tdiff(&cstart_tv, &cend_tv)
		 << endl;
	    cout << _("Average transfer speed: ")
		 << (double)backupSize / tdiff(&cstart_tv, &cend_tv) << endl;
	}
    }
}

void
usage(ostream *hlp)
{
    if (hlp == &cout) {
	*hlp <<
	    _("Usage: plpbackup OPTIONS [<drive>:] [<drive>:] ...\n"
	      "\n"
	      "  Options:\n"
	      "    -h, --help             Print this message and exit.\n"
	      "    -V, --version          Print version and exit.\n"
	      "    -p, --port=[HOST:]PORT Connect to ncpd on HOST, PORT.\n"
	      "    -v, --verbose          Increase verbosity.\n"
	      "    -q, --quiet            Decrease verbosity.\n"
	      "    -f, --full             Do a full backup (incremental otherwise).\n"
	      "    -b, --backup[=TGZ]     Backup to specified archive TGZ.\n"
	      "    -r, --restore=TGZ      Restore from specified archive TGZ.\n"
	      "    -F, --format           Format drive (can be combined with restore).\n"
	      "\n"
	      "  <drive> A drive character. If none given, scan all drives.\n"
	      "\n");
	exit(0);
    } else {
	*hlp << _("Try 'plpbackup --help' for more information.") << endl;
	exit(1);
    }
}

static struct option opts[] = {
    { "full",    no_argument,       0, 'f' },
    { "help",    no_argument,       0, 'h' },
    { "port",    required_argument, 0, 'p' },
    { "verbose", no_argument,       0, 'v' },
    { "quiet",   no_argument,       0, 'q' },
    { "backup",  optional_argument, 0, 'b' },
    { "restore", required_argument, 0, 'r' },
    { "format",  no_argument,       0, 'F' },
    { "version", no_argument,       0, 'V' },
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
    while ((op = getopt_long(argc, argv, "fFhqvVp:r:b::", opts, NULL)) != EOF) {
	switch (op) {
	    case 'V':
		cout << _("plpbackup version ") << VERSION << endl;
		exit(0);
	    case 'h':
		usage(&cout);
		break;
	    case 'f':
		full = true;
		break;
	    case 'v':
		verbose++;
		break;
	    case 'q':
		verbose--;
		break;
	    case 'b':
		doBackup = true;
		if (optarg)
		    archList.push_back(optarg);
		break;
	    case 'r':
		doRestore = true;
		archList.push_back(optarg);
		break;
	    case 'F':
		doFormat = true;
		break;
	    case 'p':
		parse_destination(optarg, &host, &sockNum);
		break;
	    default:
		usage(&cerr);
	}
    }
    for (int i = optind; i < argc; i++) {
	if ((strlen(argv[i]) != 2) ||
	    (toupper(argv[i][0]) < 'A') ||
	    (toupper(argv[i][0]) > 'Z') ||
	    (argv[i][1] != ':')) {
	    cerr << _("Invalid drive argument ") << argv[i] << endl;
	    usage(&cerr);
	}
	driveList.push_back(argv[i]);
    }
    if (doBackup && (doRestore || doFormat)) {
	cerr << _("Backup mode can not be combined with format or restore.")
	     << endl;
	usage(&cerr);
    }
    if (doFormat && driveList.empty()) {
	cerr << _("Format mode needs at least one drive specified.") << endl;
	usage(&cerr);
    }
    if (doBackup && (archList.size() > 1)) {
	cerr << _("Backup can only create one archive at a time.") << endl;
	usage(&cerr);
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
    if (machType == rpcs::PSI_MACH_S5) {
	rpcs::machineInfo mi;
	if ((res = Rpcs->getMachineInfo(mi)) ==
	    rfsv::E_PSI_GEN_NONE) {
	    if (!strcmp(mi.machineName, "SERIES5mx"))
		S5mx = true;
	}
    }
    if (doBackup)
	runBackup();
    if (doRestore)
	runRestore();
    if (doFormat)
	runFormat();
    delete Rpcs;
    delete Rfsv;
    return 0;
}
