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
#ifndef _ftp_h_
#define _ftp_h_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "rfsv.h"
#include "Enum.h"

class rpcs;
class bufferStore;
class bufferArray;

class ftp {
	public:
	ftp();
	~ftp();
        int session(rfsv & a, rpcs & r, rclip & rc, ppsocket & rclipSocket, int xargc, char **xargv);
        bool canClip;

	private:
	void getCommand(int &argc, char **argv);
	void initReadline(void);
        int putClipText(rpcs & r, rfsv & a, rclip & rc, ppsocket & rclipSocket, const char *data);
        bool checkClipConnection(rfsv &a, rclip & rc, ppsocket & rclipSocket);

	// utilities
	bool unixDirExists(const char *dir);
	void getUnixDir(bufferArray & files);
	void resetUnixPwd();
	void usage();
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
};

#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
