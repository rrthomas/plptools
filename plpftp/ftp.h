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

class rfsv32;
class bufferStore;
class bufferArray;

#define DEFAULT_DRIVE "C:"
#define DEFAULT_BASE_DIRECTORY "\\"

class ftp {
	public:
	ftp();
	~ftp();
	int session(rfsv32 & a, int xargc, char **xargv);

	 private:
	void getCommand(int &argc, char **argv);

	// utilities
	bool unixDirExists(const char *dir);
	void getUnixDir(bufferArray & files);
	void resetUnixPwd();
	void usage();
	void errprint(long errcode, rfsv32 & a);
	void cd(const char *source, const char *cdto, char *dest);
	int convertName(const char *orig, char *retVal);

	char localDir[1024];
	char psionDir[1024];
};

#endif
