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

#ifndef _ftp_h_
#define _ftp_h_

#include "bool.h"

class rfsv;
class rpcs;
class bufferStore;
class bufferArray;

class ftp {
	public:
	ftp();
	~ftp();
	int session(rfsv & a, rpcs & r, int xargc, char **xargv);

	 private:
	void getCommand(int &argc, char **argv);
	void initReadline(void);

	// utilities
	bool unixDirExists(const char *dir);
	void getUnixDir(bufferArray & files);
	void resetUnixPwd();
	void usage();
	void errprint(long errcode, rfsv & a);
	void cd(const char *source, const char *cdto, char *dest);

	// MJG: note, this isn't actually used anywhere
	int convertName(const char *orig, char *retVal);
#ifdef HAVE_READLINE
	char *filename_generator(char *, int);
	char *command_generator(char *, int);
	char **do_completion(char *, int, int);
#endif
	char defDrive[9];
	char localDir[1024];
	// char psionDir[1024];
};

#endif
