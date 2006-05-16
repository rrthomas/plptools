/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999  Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
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
#undef EXPERIMENTAL

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <plpintl.h>
#include <rfsv.h>
#include <rpcs.h>
#include <bufferarray.h>
#include <bufferstore.h>
#include <Enum.h>

#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <signal.h>

#include "ftp.h"

#if HAVE_LIBREADLINE
extern "C"  {
#include <readline/readline.h>
#if HAVE_LIBHISTORY
#include <readline/history.h>
#endif
#include "rlcrap.h"
}
#endif

using namespace std;

static char psionDir[1024];
static rfsv *comp_a;
static int continueRunning;


void ftp::
resetUnixPwd()
{
    getcwd(localDir, 500);
    strcat(localDir, "/");
}

ftp::ftp()
{
    resetUnixPwd();
}

ftp::~ftp()
{
}

void ftp::usage() {
    cout << _("Known FTP commands:") << endl << endl;
    cout << "  pwd" << endl;
    cout << "  ren <oldname> <newname>" << endl;
    cout << "  touch <psionfile>" << endl;
    cout << "  gtime <psionfile>" << endl;
    cout << "  test <psionfile>" << endl;
    cout << "  gattr <psionfile>" << endl;
    cout << "  sattr [[-|+]rwhsa] <psionfile>" << endl;
    cout << "  devs" << endl;
    cout << "  dir|ls" << endl;
    cout << "  dircnt" << endl;
    cout << "  cd <dir>" << endl;
    cout << "  lcd <dir>" << endl;
    cout << "  !<system command>" << endl;
    cout << "  get <psionfile>" << endl;
    cout << "  put <unixfile>" << endl;
    cout << "  mget <shellpattern>" << endl;
    cout << "  mput <shellpattern>" << endl;
    cout << "  cp <psionfile> <psionfile>" << endl;
    cout << "  del|rm <psionfile>" << endl;
    cout << "  mkdir <psiondir>" << endl;
    cout << "  rmdir <psiondir>" << endl;
    cout << "  volname <drive> <name>" << endl;
    cout << "  prompt" << endl;
    cout << "  hash" << endl;
    cout << "  bye" << endl;
    cout << endl << _("Known RPC commands:") << endl << endl;
    cout << "  ps" << endl;
    cout << "  kill <pid|'all'>" << endl;
    cout << "  run <psionfile> [args]" << endl;
    cout << "  killsave <unixfile>" << endl;
    cout << "  runrestore <unixfile>" << endl;
    cout << "  machinfo" << endl;
    cout << "  ownerinfo" << endl;
    cout << "  settime" << endl;
    cout << "  setupinfo" << endl;
}

static int Wildmat(const char *s, char *p);

static int
Star(const char *s, char *p)
{
    while (Wildmat(s, p) == 0)
	if (*++s == '\0')
	    return 0;
    return 1;
}

static int
Wildmat(const char *s, char *p)
{
    register int last;
    register int matched;
    register int reverse;

    for (; *p; s++, p++)
	switch (*p) {
	    case '\\':
		/*
		* Literal match with following character,
		* fall through.
		*/
		p++;
	    default:
		if (*s != *p)
		    return (0);
		continue;
	    case '?':
		/* Match anything. */
		if (*s == '\0')
		    return (0);
		continue;
	    case '*':
		/* Trailing star matches everything. */
		return (*++p ? Star(s, p) : 1);
	    case '[':
		/* [^....] means inverse character class. */
		if ((reverse = (p[1] == '^')))
		    p++;
		for (last = 0, matched = 0; *++p && (*p != ']'); last = *p)
		    /* This next line requires a good C compiler. */
		    if (*p == '-' ? *s <= *++p && *s >= last : *s == *p)
			matched = 1;
		if (matched == reverse)
		    return (0);
		continue;
	}
    return (*s == '\0');
}

static int
checkAbortNoHash(void *, u_int32_t)
{
    return continueRunning;
}

static int
checkAbortHash(void *, u_int32_t)
{
    if (continueRunning) {
	printf("#"); fflush(stdout);
    }
    return continueRunning;
}

static RETSIGTYPE
sigint_handler(int i) {
    continueRunning = 0;
    signal(SIGINT, sigint_handler);
}

static RETSIGTYPE
sigint_handler2(int i) {
    continueRunning = 0;
    fclose(stdin);
    signal(SIGINT, sigint_handler2);
}

int ftp::
session(rfsv & a, rpcs & r, int xargc, char **xargv)
{
    int argc;
    char *argv[10];
    char f1[256];
    char f2[256];
    Enum<rfsv::errs> res;
    bool prompt = true;
    bool hash = false;
    cpCallback_t cab = checkAbortNoHash;
    bool once = false;

    if (xargc) {
	once = true;
	argc = (xargc<10)?xargc:10;
	for (int i = 0; i < argc; i++)
	    argv[i] = xargv[i];
    }
    {
	Enum<rpcs::machs> machType;
	bufferArray b;
	if ((res = r.getOwnerInfo(b)) == rfsv::E_PSI_GEN_NONE) {
	    r.getMachineType(machType);
	    if (!once) {
		int speed = a.getSpeed();
		cout << _("Connected to a ") << machType << _(" at ")
		     << speed << _(" baud, OwnerInfo:") << endl;
		while (!b.empty())
		    cout << "  " << b.pop().getString() << endl;
		cout << endl;
	    }
	} else
	    cerr << _("OwnerInfo returned error ") << res << endl;
    }

    if (!strcmp(DDRIVE, "AUTO")) {
	u_int32_t devbits;
	int i;

	strcpy(defDrive, "::");
	if (a.devlist(devbits) == rfsv::E_PSI_GEN_NONE) {

	    for (i = 0; i < 26; i++) {
		PlpDrive drive;
		if ((devbits & 1) && a.devinfo(i + 'A', drive) == rfsv::E_PSI_GEN_NONE) {
		    defDrive[0] = 'A' + i;
		    break;
		}
		devbits >>= 1;
	    }
	}
	if (!strcmp(defDrive, "::")) {
	    cerr << _("FATAL: Couldn't find default Drive") << endl;
	    return -1;
	}
    } else
	strcpy(defDrive, DDRIVE);
    strcpy(psionDir, defDrive);
    strcat(psionDir, DBASEDIR);
    comp_a = &a;
    if (!once) {
	cout << _("Psion dir is: \"") << psionDir << "\"" << endl;
	initReadline();
    }
    continueRunning = 1;
    signal(SIGINT, sigint_handler);
    do {
	if (!once)
	    getCommand(argc, argv);

	if ((!strcmp(argv[0], "help")) || (!strcmp(argv[0], "?"))) {
	    usage();
	    continue;
	}
	if (!strcmp(argv[0], "prompt")) {
	    prompt = !prompt;
	    cout << _("Prompting now ") << (prompt? _("on") : _("off")) << endl;
	    continue;
	}
	if (!strcmp(argv[0], "hash")) {
	    hash = !hash;
	    cout << _("Hash printing now ") << (hash? _("on") : _("off")) << endl;
	    cab = (hash) ? checkAbortHash : checkAbortNoHash;
	    continue;
	}
	if (!strcmp(argv[0], "pwd")) {
	    cout << _("Local dir: \"") << localDir << "\"" << endl;
	    cout << _("Psion dir: \"") << psionDir << "\"" << endl;
	    continue;
	}
	if (!strcmp(argv[0], "volname") && (argc == 3) && (strlen(argv[1]) == 1)) {
	    if ((res = a.setVolumeName(toupper(argv[1][0]), argv[2])) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    continue;

	}
	if (!strcmp(argv[0], "ren") && (argc == 3)) {
	    strcpy(f1, psionDir);
	    strcat(f1, argv[1]);
	    strcpy(f2, psionDir);
	    strcat(f2, argv[2]);
	    if ((res = a.rename(f1, f2)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    continue;
	}
	if (!strcmp(argv[0], "cp") && (argc == 3)) {
	    strcpy(f1, psionDir);
	    strcat(f1, argv[1]);
	    strcpy(f2, psionDir);
	    strcat(f2, argv[2]);
	    if ((res = a.copyOnPsion(f1, f2, NULL, cab)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    continue;
	}
	if (!strcmp(argv[0], "touch") && (argc == 2)) {
	    strcpy(f1, psionDir);
	    strcat(f1, argv[1]);
	    PsiTime pt;
	    if ((res = a.fsetmtime(f1, pt)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    continue;
	}
	if (!strcmp(argv[0], "test") && (argc == 2)) {
	    PlpDirent e;
	    strcpy(f1, psionDir);
	    strcat(f1, argv[1]);
	    if ((res = a.fgeteattr(f1, e)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    else
		cout << e << endl;
	    continue;
	}
	if (!strcmp(argv[0], "gattr") && (argc == 2)) {
	    u_int32_t attr;
	    strcpy(f1, psionDir);
	    strcat(f1, argv[1]);
	    if ((res = a.fgetattr(f1, attr)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    else {
		cout << hex << setw(4) << setfill('0') << attr;
		cout << " (" << a.attr2String(attr) << ")" << endl;
	    }
	    continue;
	}
	if (!strcmp(argv[0], "gtime") && (argc == 2)) {
	    PsiTime mtime;
	    strcpy(f1, psionDir);
	    strcat(f1, argv[1]);
	    if ((res = a.fgetmtime(f1, mtime)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    else
		cout << mtime << "(" << hex
		     << setw(8) << setfill('0') << mtime.getPsiTimeHi()
		     << ":"
		     << setw(8) << setfill('0') << mtime.getPsiTimeLo()
		     << ")" << endl;
	    continue;
	}
	if (!strcmp(argv[0], "sattr") && (argc == 3)) {
	    long attr[2];
	    int aidx = 0;
	    char *p = argv[1];

	    strcpy(f1, psionDir);
	    strcat(f1, argv[2]);

	    attr[0] = attr[1] = 0;
	    while (*p) {
		switch (*p) {
		    case '+':
			aidx = 0;
			break;
		    case '-':
			aidx = 1;
			break;
		    case 'r':
			attr[aidx] |= rfsv::PSI_A_READ;
			attr[aidx] &= ~rfsv::PSI_A_READ;
			break;
		    case 'w':
			attr[1 - aidx] |= rfsv::PSI_A_RDONLY;
			attr[aidx] &= ~rfsv::PSI_A_RDONLY;
			break;
		    case 'h':
			attr[aidx] |= rfsv::PSI_A_HIDDEN;
			attr[1 - aidx] &= ~rfsv::PSI_A_HIDDEN;
			break;
		    case 's':
			attr[aidx] |= rfsv::PSI_A_SYSTEM;
			attr[1 - aidx] &= ~rfsv::PSI_A_SYSTEM;
			break;
		    case 'a':
			attr[aidx] |= rfsv::PSI_A_ARCHIVE;
			attr[1 - aidx] &= ~rfsv::PSI_A_ARCHIVE;
			break;
		}
		p++;
	    }
	    if ((res = a.fsetattr(f1, attr[0], attr[1])) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    continue;
	}
	if (!strcmp(argv[0], "dircnt")) {
	    u_int32_t cnt;
	    if ((res = a.dircount(psionDir, cnt)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    else
		cout << cnt << _(" Entries") << endl;
	    continue;
	}
	if (!strcmp(argv[0], "devs")) {
	    u_int32_t devbits;
	    if ((res = a.devlist(devbits)) == rfsv::E_PSI_GEN_NONE) {
		cout << _("Drive Type Volname     Total     Free      UniqueID") << endl;
		for (int i = 0; i < 26; i++) {
		    PlpDrive drive;

		    if ((devbits & 1) != 0) {
			if (a.devinfo(i + 'A', drive) == rfsv::E_PSI_GEN_NONE)
			    cout << (char) ('A' + i) << "     " << hex
				 << setw(4) << setfill('0')
				 << drive.getMediaType() << " " << setw(12)
				 << setfill(' ') << setiosflags(ios::left)
				 << drive.getName().c_str()
				 << resetiosflags(ios::left) << dec << setw(9)
				 << drive.getSize() << setw(9)
				 << drive.getSpace() << "  " << setw(8)
				 << setfill('0') << hex << drive.getUID()
				 << dec << endl;
		    }
		    devbits >>= 1;
		}
	    } else
		cerr << _("Error: ") << res << endl;
	    continue;
	}
	if (!strcmp(argv[0], "ls") || !strcmp(argv[0], "dir")) {
	    PlpDir files;
	    char dtmp[1024];
	    char *dname = psionDir;

	    if (argc > 1) {
		u_int32_t tmp;
		strcpy(dtmp, psionDir);
		if (!strcmp(argv[1], "..")) {
		    strcpy(f1, psionDir);
		    char *p = f1 + strlen(f1);
		    if (p > f1)
			p--;
		    *p = '\0';
		    while ((p > f1) && (*p != '/') && (*p != '\\'))
			p--;
		    *(++p) = '\0';
		    if (strlen(f1) < 3) {
			strcpy(f1, psionDir);
			f1[3] = '\0';
		    }
		} else {
		    if ((argv[1][0] != '/') && (argv[1][0] != '\\') &&
			(argv[1][1] != ':')) {
			strcpy(f1, psionDir);
			strcat(f1, argv[1]);
		    } else
			strcpy(f1, argv[1]);
		}
		if ((f1[strlen(f1) -1] != '/') && (f1[strlen(f1) -1] != '\\'))
		    strcat(f1,"\\");
		for (char *p = f1; *p; p++)
		    if (*p == '/')
			*p = '\\';
		strcpy(dtmp, f1);
		dname = dtmp;
	    }

	    if ((res = a.dir(dname, files)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    else
		while (!files.empty()) {
		    cout << files[0] << endl;
		    files.pop_front();
		}
	    continue;
	}
	if (!strcmp(argv[0], "lcd")) {
	    if (argc == 1)
		resetUnixPwd();
	    else {
		if (chdir(argv[1]) == 0) {
		    getcwd(localDir, sizeof(localDir));
		    strcat(localDir, "/");
		} else
		    cerr << _("No such directory") << endl
			 << _("Keeping original directory \"") << localDir << "\"" << endl;
	    }
	    continue;
	}
	if (!strcmp(argv[0], "cd")) {
	    if (argc == 1) {
		strcpy(psionDir, defDrive);
		strcat(psionDir, DBASEDIR);
	    } else {
		u_int32_t tmp;
		if (!strcmp(argv[1], "..")) {
		    strcpy(f1, psionDir);
		    char *p = f1 + strlen(f1);
		    if (p > f1)
			p--;
		    *p = '\0';
		    while ((p > f1) && (*p != '/') && (*p != '\\'))
			p--;
		    *(++p) = '\0';
		    if (strlen(f1) < 3) {
			strcpy(f1, psionDir);
			f1[3] = '\0';
		    }
		} else {
		    if ((argv[1][0] != '/') && (argv[1][0] != '\\') &&
			(argv[1][1] != ':')) {
			strcpy(f1, psionDir);
			strcat(f1, argv[1]);
		    } else
			strcpy(f1, argv[1]);
		}
		if ((f1[strlen(f1) -1] != '/') && (f1[strlen(f1) -1] != '\\'))
		    strcat(f1,"\\");
		if ((res = a.dircount(f1, tmp)) == rfsv::E_PSI_GEN_NONE) {
		    for (char *p = f1; *p; p++)
			if (*p == '/')
			    *p = '\\';
		    strcpy(psionDir, f1);
		}
		else {
		    cerr << _("Error: ") << res << endl;
		    cerr << _("Keeping original directory \"") << psionDir << "\"" << endl;
		}
	    }
	    continue;
	}
	if ((!strcmp(argv[0], "get")) && (argc > 1)) {
	    struct timeval stime;
	    struct timeval etime;
	    struct stat stbuf;

	    strcpy(f1, psionDir);
	    strcat(f1, argv[1]);
	    strcpy(f2, localDir);
	    if (argc == 2)
		strcat(f2, argv[1]);
	    else
		strcat(f2, argv[2]);
	    gettimeofday(&stime, 0L);
	    if ((res = a.copyFromPsion(f1, f2, NULL, cab)) != rfsv::E_PSI_GEN_NONE) {
		if (hash)
		    cout << endl;
		continueRunning = 1;
		cerr << _("Error: ") << res << endl;
	    } else {
		if (hash)
		    cout << endl;
		gettimeofday(&etime, 0L);
		long dsec = etime.tv_sec - stime.tv_sec;
		long dhse = (etime.tv_usec / 10000) -
		    (stime.tv_usec /10000);
		if (dhse < 0) {
		    dsec--;
		    dhse = 100 + dhse;
		}
		float dt = dhse;
		dt /= 100.0;
		dt += dsec;
		stat(f2, &stbuf);
		float cps = (float)(stbuf.st_size) / dt;
		cout << _("Transfer complete, (") << dec << stbuf.st_size
		     << _(" bytes in ") << dsec << "."
		     << dhse << _(" secs = ") << cps << " cps)\n";
	    }
	    continue;
	} else if ((!strcmp(argv[0], "mget")) && (argc == 2)) {
	    char *pattern = argv[1];
	    PlpDir files;
	    if ((res = a.dir(psionDir, files)) != rfsv::E_PSI_GEN_NONE) {
		cerr << _("Error: ") << res << endl;
		continue;
	    }
	    for (int i = 0; i < files.size(); i++) {
		PlpDirent e = files[i];
		char temp[100];
		long attr = e.getAttr();

		if (attr & (rfsv::PSI_A_DIR | rfsv::PSI_A_VOLUME))
		    continue;
		if (!Wildmat(e.getName(), pattern))
		    continue;
		do {
		    cout << _("Get \"") << e.getName() << "\" (y,n): ";
		    if (prompt) {
			cout.flush();
			cin.getline(temp, 100);
		    } else {
			temp[0] = 'y';
			temp[1] = 0;
			cout << "y ";
			cout.flush();
		    }
		} while (temp[1] != 0 || (temp[0] != 'y' && temp[0] != 'n'));
		if (temp[0] != 'n') {
		    strcpy(f1, psionDir);
		    strcat(f1, e.getName());
		    strcpy(f2, localDir);
		    strcat(f2, e.getName());
		    if (temp[0] == 'l') {
			for (char *p = f2; *p; p++)
			    *p = tolower(*p);
		    }
		    if ((res = a.copyFromPsion(f1, f2, NULL, cab)) != rfsv::E_PSI_GEN_NONE) {
			if (hash)
			    cout << endl;
			continueRunning = 1;
			cerr << _("Error: ") << res << endl;
			break;
		    } else {
			if (hash)
			    cout << endl;
			cout << _("Transfer complete\n");
		    }
		}
	    }
	    continue;
	}
	if (!strcmp(argv[0], "put") && (argc >= 2)) {
	    struct timeval stime;
	    struct timeval etime;
	    struct stat stbuf;

	    strcpy(f1, localDir);
	    strcat(f1, argv[1]);
	    strcpy(f2, psionDir);
	    if (argc == 2)
		strcat(f2, argv[1]);
	    else
		strcat(f2, argv[2]);
	    gettimeofday(&stime, 0L);
	    if ((res = a.copyToPsion(f1, f2, NULL, cab)) != rfsv::E_PSI_GEN_NONE) {
		if (hash)
		    cout << endl;
		continueRunning = 1;
		cerr << _("Error: ") << res << endl;
	    } else {
		if (hash)
		    cout << endl;
		gettimeofday(&etime, 0L);
		long dsec = etime.tv_sec - stime.tv_sec;
		long dhse = (etime.tv_usec / 10000) -
		    (stime.tv_usec /10000);
		if (dhse < 0) {
		    dsec--;
		    dhse = 100 + dhse;
		}
		float dt = dhse;
		dt /= 100.0;
		dt += dsec;
		stat(f1, &stbuf);
		float cps = (float)(stbuf.st_size) / dt;
		cout << _("Transfer complete, (") << dec << stbuf.st_size
		     << _(" bytes in ") << dsec << "."
		     << dhse << _(" secs = ") << cps << " cps)\n";
	    }
	    continue;
	}
	if ((!strcmp(argv[0], "mput")) && (argc == 2)) {
	    char *pattern = argv[1];
	    DIR *d = opendir(localDir);
	    if (d) {
		struct dirent *de;
		do {
		    de = readdir(d);
		    if (de) {
			char temp[100];
			struct stat st;

			if (!Wildmat(de->d_name, pattern))
			    continue;
			strcpy(f1, localDir);
			strcat(f1, de->d_name);
			if (stat(f1, &st) != 0)
			    continue;
			if (!S_ISREG(st.st_mode))
			    continue;
			do {
			    cout << _("Put \"") << de->d_name << "\" y,n: ";
			    if (prompt) {
				cout.flush();
				cin.getline(temp, 100);
			    } else {
				temp[0] = 'y';
				temp[1] = 0;
				cout << "y ";
				cout.flush();
			    }
			} while (temp[1] != 0 || (temp[0] != 'y' && temp[0] != 'n'));
			if (temp[0] == 'y') {
			    strcpy(f2, psionDir);
			    strcat(f2, de->d_name);
			    if ((res = a.copyToPsion(f1, f2, NULL, cab)) != rfsv::E_PSI_GEN_NONE) {
				if (hash)
				    cout << endl;
				continueRunning = 1;
				cerr << _("Error: ") << res << endl;
				break;
			    } else {
				if (hash)
				    cout << endl;
				cout << _("Transfer complete\n");
			    }
			}
		    }
		} while (de);
		closedir(d);
	    } else
		cerr << _("Error in directory name \"") << localDir << "\"\n";
	    continue;
	}
	if ((!strcmp(argv[0], "del") ||
	     !strcmp(argv[0], "rm")) && (argc == 2)) {
	    strcpy(f1, psionDir);
	    strcat(f1, argv[1]);
	    if ((res = a.remove(f1)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    continue;
	}
	if (!strcmp(argv[0], "mkdir") && (argc == 2)) {
	    strcpy(f1, psionDir);
	    strcat(f1, argv[1]);
	    if ((res = a.mkdir(f1)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    continue;
	}
	if (!strcmp(argv[0], "rmdir") && (argc == 2)) {
	    strcpy(f1, psionDir);
	    strcat(f1, argv[1]);
	    if ((res = a.rmdir(f1)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    continue;
	}
	if (argv[0][0] == '!') {
	    char cmd[1024];
	    strcpy(cmd, &argv[0][1]);
	    for (int i=1; i<argc; i++) {
		strcat(cmd, " ");
		strcat(cmd, argv[i]);
	    }
	    if (strlen(cmd))
		system(cmd);
	    else {
		char *sh;
		cout << _("Starting subshell ...\n");
		sh = getenv("SHELL");
		if (!sh)
		    sh = "/bin/sh";
		system(sh);
	    }
	    continue;
	}
	// RPCS commands
#ifdef EXPERIMENTAL
	if (!strcmp(argv[0], "x")) {
	    u_int16_t hhh;
	    if (r.regOpenIter((u_int32_t)-1, "%PDF-", hhh)
		== rfsv::E_PSI_GEN_NONE) {
		Enum<rfsv::errs> res;
		while ((res = r.regReadIter(hhh)) == rfsv::E_PSI_GEN_NONE)
			;;
	    }
	    continue;
	}
#endif
	if (!strcmp(argv[0], "settime")) {
	    if ((res = r.setTime(time(NULL))) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
            continue;
        }
	if (!strcmp(argv[0], "setupinfo")) {
	    Enum<rfsv::errs> res;
	    bufferStore db;

	    if ((res = r.configRead(0, db)) != rfsv::E_PSI_GEN_NONE) {
		cerr << _("Error: ") << res << endl;
		continue;
	    }
	    if (db.getLen() < 1152) {
		cerr << _("Unknown setup info received") << endl;
		continue;
	    }
	    cout << _("Setup information:") << endl;
	    cout << _("  Screen contrast:                  ") << dec
		 << db.getDWord(0x4c) + 1 << endl;
	    cout << _("  Keyboard click:                   ")
		 << (db.getDWord(0x200) ?
		(db.getDWord(0x204) ? _("high") : _("low")) : _("off")) << endl;
	    cout << _("  Screen click:                     ")
		 << (db.getDWord(0x20c) ?
		(db.getDWord(0x210) ? _("high") : _("low")) : _("off")) << endl;
	    cout << _("  Error sound:                      ")
		 << (db.getDWord(0x214) ?
		(db.getDWord(0x218) ? _("high") : _("low")) : _("off")) << endl;
	    cout << _("  Auto-switch off:                  ");
	    switch (db.getDWord(0x228)) {
		case 0:
		    cout << _("never");
		    break;
		case 1:
		    cout << _("if running on battery power");
		    break;
		case 2:
		    cout << _("always");
		    break;
	    }
	    cout << endl;
	    if (db.getDWord(0x228) != 0)
		cout << _("  Switch off after:                 ") << dec
		     << db.getDWord(0x22c) << _(" seconds") << endl;
	    cout << _("  Backlight off after:              ") << dec
		 << db.getDWord(0x234) << _(" seconds") << endl;
	    cout << _("  Switch on when tapping on screen: ")
		 << (db.getDWord(0x238) ? _("yes") : _("no")) << endl;
	    cout << _("  Switch on when opening:           ")
		 << (db.getDWord(0x23c) ? _("yes") : _("no")) << endl;
	    cout << _("  Switch off when closing:          ")
		 << (db.getDWord(0x23c) ? _("yes") : _("no")) << endl;
	    cout << _("  Ask for password on startup:      ")
		 << (db.getDWord(0x29c) ? _("yes") : _("no")) << endl;
	    cout << _("  Show Owner info on startup:       ");
	    switch (db.getByte(0x3b0)) {
		case 0x31:
		    cout << _("never");
		    break;
		case 0x32:
		    cout << _("once a day");
		    break;
		case 0x33:
		    cout << _("always");
		    break;
	    }
	    cout << endl;
	    continue;
	}
	if (!strcmp(argv[0], "run") && (argc >= 2)) {
	    char argbuf[1024];
	    char cmdbuf[1024];

	    argbuf[0] = 0;
	    for (int i = 2; i < argc; i++) {
		if (i > 2) {
		    strcat(argbuf, " ");
		    strcat(argbuf, argv[i]);
		} else
		    strcpy(argbuf, argv[i]);
	    }
	    if (!strchr(argv[1], ':')) {
		strcpy(cmdbuf, psionDir);
		strcat(cmdbuf, argv[1]);
	    } else
		strcpy(cmdbuf, argv[1]);
	    r.execProgram(cmdbuf, argbuf);
	    continue;
	}
	if (!strcmp(argv[0], "ownerinfo")) {
	    bufferArray b;
	    if ((res = r.getOwnerInfo(b)) != rfsv::E_PSI_GEN_NONE) {
		cerr << _("Error: ") << res << endl;
		continue;
	    }
	    while (!b.empty())
		cout << "  " << b.pop().getString() << endl;
	    continue;
	}
	if (!strcmp(argv[0], "machinfo")) {
	    rpcs::machineInfo mi;
	    if ((res = r.getMachineInfo(mi)) != rfsv::E_PSI_GEN_NONE) {
		cerr << _("Error: ") << res << endl;
		continue;
	    }

	    cout << _("General:") << endl;
	    cout << _("  Machine Type: ") << mi.machineType << endl;
	    cout << _("  Machine Name: ") << mi.machineName << endl;
	    cout << _("  Machine UID:  ") << hex << mi.machineUID << dec << endl;
	    cout << _("  UI Language:  ") << mi.uiLanguage << endl;
	    cout << _("ROM:") << endl;
	    cout << _("  Version:      ") << mi.romMajor << "." << setw(2) << setfill('0') <<
		mi.romMinor << "(" << mi.romBuild << ")" << endl;
	    cout << _("  Size:         ") << mi.romSize / 1024 << "k" << endl;
	    cout << _("  Programmable: ") <<
		(mi.romProgrammable ? _("yes") : _("no")) << endl;
	    cout << _("RAM:") << endl;
	    cout << _("  Size:         ") << mi.ramSize / 1024 << "k" << endl;
	    cout << _("  Free:         ") << mi.ramFree / 1024 << "k" << endl;
	    cout << _("  Free max:     ") << mi.ramMaxFree / 1024 << "k" << endl;
	    cout << _("RAM disk size:  ") << mi.ramDiskSize / 1024 << "k" << endl;
	    cout << _("Registry size:  ") << mi.registrySize << endl;
	    cout << _("Display size:   ") << mi.displayWidth << "x" <<
		mi.displayHeight << endl;
	    cout << _("Time:") << endl;
	    PsiTime pt(&mi.time, &mi.tz);
	    cout << _("  Current time: ") << pt << endl;
	    cout << _("  UTC offset:   ") << mi.tz.utc_offset << _(" seconds") << endl;
	    cout << _("  DST:          ") <<
		(mi.tz.dst_zones & PsiTime::PSI_TZ_HOME ? _("yes") : _("no")) << endl;
	    cout << _("  Timezone:     ") << mi.tz.home_zone << endl;
	    cout << _("  Country Code: ") << mi.countryCode << endl;
	    cout << _("Main battery:") << endl;
	    pt.setPsiTime(&mi.mainBatteryInsertionTime);
	    cout << _("  Changed at:   ") << pt << endl;
	    cout << _("  Used for:     ") << mi.mainBatteryUsedTime << endl;
	    cout << _("  Status:       ") << mi.mainBatteryStatus << endl;
	    cout << _("  Current:      ") << mi.mainBatteryCurrent << " mA" << endl;
	    cout << _("  UsedPower:    ") << mi.mainBatteryUsedPower << " mAs" << endl;
	    cout << _("  Voltage:      ") << mi.mainBatteryVoltage << " mV" << endl;
	    cout << _("  Max. voltage: ") << mi.mainBatteryMaxVoltage << " mV" << endl;
	    cout << _("Backup battery:") << endl;
	    cout << _("  Status:       ") << mi.backupBatteryStatus << endl;
	    cout << _("  Voltage:      ") << mi.backupBatteryVoltage << " mV" << endl;
	    cout << _("  Max. voltage: ") << mi.backupBatteryMaxVoltage << " mV" << endl;
	    cout << _("External power:") << endl;
	    cout << _("  Supplied:     ")
		 << (mi.externalPower ? _("yes") : _("no")) << endl;
	    cout << _("  Used for:     ") << mi.externalPowerUsedTime << endl;
	    continue;
	}
	if (!strcmp(argv[0], "runrestore") && (argc == 2)) {
	    ifstream ip(argv[1]);
	    char cmd[512];
	    char arg[512];

	    if (!ip) {
		cerr << _("Could not read processlist ") << argv[1] << endl;
		continue;
	    }
	    ip >> cmd >> arg;

	    if (strcmp(cmd, "#plpftp") || strcmp(arg, "processlist")) {
		ip.close();
		cerr << _("Error: ") << argv[1] <<
		    _(" is not a process list saved with killsave") << endl;
		continue;
	    }
	    while (!ip.eof()) {
		ip >> cmd >> arg;
		ip.get(&arg[strlen(arg)], sizeof(arg) - strlen(arg), '\n');
		// cout << "cmd=\"" << cmd << "\" arg=\"" << arg << "\"" << endl;
		if (strlen(cmd) > 0) {
		    // Workaround for broken programs like Backlite. These do not store
		    // the full program path. In that case we try running the arg1 which
		    // results in starting the program via recog. facility.
		    if ((strlen(arg) > 2) && (arg[1] == ':') && (arg[0] >= 'A') &&
			(arg[0] <= 'Z'))
			res = r.execProgram(arg, "");
		    else
			res = r.execProgram(cmd, arg);
		    if (res != rfsv::E_PSI_GEN_NONE) {
			// If we got an error here, that happened probably because
			// we have no path at all (e.g. Macro5) and the program is not
			// registered in the Psion's path properly. Now try the ususal
			// \System\Apps\<AppName>\<AppName>.app on all drives.
			if (strchr(cmd, '\\') == NULL) {
			    u_int32_t devbits;
			    char tmp[512];
			    if ((res = a.devlist(devbits)) == rfsv::E_PSI_GEN_NONE) {
				int i;
				for (i = 0; i < 26; i++) {
				    if (devbits & 1) {
					sprintf(tmp,
						"%c:\\System\\Apps\\%s\\%s.app",
						'A' + i, cmd, cmd);
					res = r.execProgram(tmp, "");
				    }
				    if (res == rfsv::E_PSI_GEN_NONE)
					break;
				}
			    }
			}
		    }
		    if (res != rfsv::E_PSI_GEN_NONE) {
			cerr << _("Could not start ") << cmd << " " << arg << endl;
			cerr << _("Error: ") << res << endl;
		    }
		}
	    }
	    ip.close();
	    continue;
	}
	if (!strcmp(argv[0], "killsave") && (argc == 2)) {
	    processList tmp;
	    if ((res = r.queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    else {
		ofstream op(argv[1]);
		if (!op) {
		    cerr << _("Could not write processlist ") << argv[1] << endl;
		    continue;
		}
		op << "#plpftp processlist" << endl;
		processList::iterator i;
		for (i = tmp.begin(); i != tmp.end(); i++) {
		    op << i->getArgs() << endl;
		    r.stopProgram(i->getProcId());
		}
		op.close();
	    }
	    continue;
	}
	if (!strcmp(argv[0], "kill") && (argc >= 2)) {
	    processList tmp;
	    bool anykilled = false;
	    if ((res = r.queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    else {
		for (int i = 1; i < argc; i++) {
		    int kpid;
		    if (!strcmp(argv[i], "all"))
			kpid = -1;
		    else
			sscanf(argv[i], "%d", &kpid);
		    processList::iterator j;
		    for (j = tmp.begin(); j != tmp.end(); j++) {
			if (kpid == -1 || kpid == j->getPID()) {
			    r.stopProgram(j->getProcId());
			    anykilled = true;
			}
		    }
		    if (kpid == -1)
			break;
		}
		if (!anykilled)
		    cerr << _("no such process") << endl;
	    }
	    continue;
	}
	if (!strcmp(argv[0], "ps")) {
	    processList tmp;
	    if ((res = r.queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE)
		cerr << _("Error: ") << res << endl;
	    else {
		cout << "PID   CMD          ARGS" << endl;
		for (processList::iterator i = tmp.begin(); i != tmp.end(); i++)
		    cout << *i << endl;
	    }
	    continue;
	}
	if (strcmp(argv[0], "bye") && strcmp(argv[0], "quit"))
	    cerr << _("syntax error. Try \"help\"") << endl;
    } while (strcmp(argv[0], "bye") && strcmp(argv[0], "quit") && !once &&
	     continueRunning);
    return a.getStatus();
}

#if HAVE_LIBREADLINE
#if (READLINE_VERSION >= 402)
#define FUNCAST(f) f
#define CPFUNCAST(f) f
#define MATCHFUNCTION rl_completion_matches
#else
#define FUNCAST(f) (Function *)f
#define CPFUNCAST(f) (CPPFunction *)f
#define MATCHFUNCTION completion_matches
#endif

static char *all_commands[] = {
    "pwd", "ren", "touch", "gtime", "test", "gattr", "sattr", "devs",
    "dir", "ls", "dircnt", "cd", "lcd", "get", "put", "mget", "mput",
    "del", "rm", "mkdir", "rmdir", "prompt", "bye", "cp", "volname",
    "ps", "kill", "killsave", "runrestore", "run", "machinfo",
    "ownerinfo", "help", "settime", "setupinfo", NULL
};

static char *localfile_commands[] = {
    "lcd ", "put ", "mput ", "killsave ", "runrestore ", NULL
};

static char *remote_dir_commands[] = {
    "cd ", "rmdir ", NULL
};

static PlpDir comp_files;
static long maskAttr;
static char cplPath[1024];

static char*
filename_generator(
#if (READLINE_VERSION >= 402)
				   const char *text,
#else
				   char *text,
#endif
				   int state)
{
    static int len;
    string tmp;

    if (!state) {
	Enum<rfsv::errs> res;
	len = strlen(text);
	tmp = psionDir;
	tmp += cplPath;
	tmp = rfsv::convertSlash(tmp);
	if ((res = comp_a->dir(tmp.c_str(), comp_files)) != rfsv::E_PSI_GEN_NONE) {
	    cerr << _("Error: ") << res << endl;
	    return NULL;
	}
    }
    while (!comp_files.empty()) {
	PlpDirent e = comp_files.front();
	long attr = e.getAttr();

	comp_files.pop_front();
	if ((attr & maskAttr) == 0)
	    continue;
	tmp = cplPath;
	tmp += e.getName();
	if (!(strncmp(tmp.c_str(), text, len))) {
	    if (attr & rfsv::PSI_A_DIR) {
		rl_completion_append_character = '\0';
		tmp += '/';
	    }
	    return (strdup(tmp.c_str()));
	}
    }
    return NULL;
}

static char *
command_generator(
#if (READLINE_VERSION >= 402)
		  const char *text,
#else
		  char *text,
#endif
		  int state)
{
    static int idx, len;
    char *name;

    if (!state) {
	idx = 0;
	len = strlen(text);
    }
    while ((name = all_commands[idx])) {
	idx++;
	if (!strncmp(name, text, len))
	    return (strdup(name));
    }
    return NULL;
}

#if (READLINE_VERSION >= 402)
static char * null_completion(const char *, int) {
    return "";
}
#else
static int null_completion() {
    return 0;
}
#endif

static char **
do_completion(const char *text, int start, int end)
{
    char **matches = NULL;

    rl_completion_entry_function = FUNCAST(null_completion);
    rl_completion_append_character = ' ';
#if (READLINE_VERSION >= 402)
    rl_attempted_completion_over = 1;
#endif
    if (start == 0)
	{
#if HAVE_LIBREADLINE
# if (READLINE_VERSION >= 402)
	matches = MATCHFUNCTION(text, command_generator);
# else
	char* txt = (char*)text;
	matches = MATCHFUNCTION(txt, (CPFunction*)command_generator);
# endif
#endif
	}
    else {
	int idx = 0;
	char *name;
	char *p;

	rl_filename_quoting_desired = 1;
	while ((name = localfile_commands[idx])) {
	    idx++;
	    if (!strncmp(name, rl_line_buffer, strlen(name))) {
		rl_completion_entry_function = NULL;
		return NULL;
	    }
	}
	maskAttr = 0xffff;
	idx = 0;
	strcpy(cplPath, text);
	p = strrchr(cplPath, '/');
	if (p)
	    *(++p) = '\0';
	else
	    cplPath[0] = '\0';
	while ((name = remote_dir_commands[idx])) {
	    idx++;
	    if (!strncmp(name, rl_line_buffer, strlen(name)))
		maskAttr = rfsv::PSI_A_DIR;
	}

#if HAVE_LIBREADLINE
# if (READLINE_VERSION >= 402)
	matches = MATCHFUNCTION(text, filename_generator);
# else
	{
	char* txt = (char*)text;
	matches = MATCHFUNCTION(txt, (CPFunction*)filename_generator);
	}
# endif
#endif
    }
    return matches;
}
#endif

void ftp::
initReadline(void)
{
#if HAVE_LIBREADLINE
    rl_readline_name = "plpftp";
    rl_completion_entry_function = FUNCAST(null_completion);
    rl_attempted_completion_function = CPFUNCAST(do_completion);
#if (READLINE_VERSION < 402)
    rlcrap_setpointers(command_generator, filename_generator);
#endif
#if (READLINE_VERSION >= 402)
    rl_basic_word_break_characters = " \t\n\"\\'`@><=;|&{(";
#endif
#endif
}

void ftp::
getCommand(int &argc, char **argv)
{
    int ws, quote;

    static char buf[1024];

    buf[0] = 0; argc = 0;
    while (!strlen(buf) && continueRunning) {
	signal(SIGINT, sigint_handler2);
#if HAVE_LIBREADLINE
	char *bp = readline("> ");
	if (!bp) {
	    strcpy(buf, "bye");
	    cout << buf << endl;
	} else {
	    strcpy(buf, bp);
#if HAVE_LIBHISTORY
	    add_history(buf);
#endif
	    free(bp);
	}
#else
	cout << "> ";
	cout.flush();
	cin.getline(buf, 1023);
	if (cin.eof()) {
	    strcpy(buf, "bye");
	    cout << buf << endl;
	}
#endif
	signal(SIGINT, sigint_handler);
    }
    ws = 1; quote = 0;
    for (char *p = buf; *p; p++)
	switch (*p) {
	    case ' ':
	    case '\t':
		if (!quote) {
		    ws = 1;
		    *p = 0;
		}
		break;
	    case '"':
		quote = 1 - quote;
		if (!quote)
		    *p = 0;
		break;
	    default:
		if (ws) {
		    argv[argc++] = p;
		}
		ws = 0;
	}
}

// Unix utilities
bool ftp::
unixDirExists(const char *dir)
{
    return false;
}

void ftp::
getUnixDir(bufferArray & files)
{
}

void ftp::
cd(const char *source, const char *cdto, char *dest)
{
    if (cdto[0] == '/' || cdto[0] == '\\' || cdto[1] == ':') {
	strcpy(dest, cdto);
	char cc = dest[strlen(dest) - 1];
	if (cc != '/' && cc != '\\')
	    strcat(dest, "/");
    } else {
	char start[200];
	strcpy(start, source);

	while (*cdto) {
	    char bit[200];
	    int j;
	    for (j = 0; cdto[j] && cdto[j] != '/' && cdto[j] != '\\'; j++)
		bit[j] = cdto[j];
	    bit[j] = 0;
	    cdto += j;
	    if (*cdto)
		cdto++;

	    if (!strcmp(bit, "..")) {
		strcpy(dest, start);
		int i;
		for (i = strlen(dest) - 2; i >= 0; i--) {
		    if (dest[i] == '/' || dest[i] == '\\') {
			dest[i + 1] = 0;
			break;
		    }
		}
	    } else {
		strcpy(dest, start);
		strcat(dest, bit);
		strcat(dest, "/");
	    }
	    strcpy(start, dest);
	}
    }
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
