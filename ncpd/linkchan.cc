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

#include <stream.h>

#include "linkchan.h"
#include "bufferstore.h"

linkChan::linkChan(ncp * _ncpController):channel(_ncpController)
{
}

void linkChan::
ncpDataCallback(bufferStore & a)
{
	if (verbose & LINKCHAN_DEBUG_LOG) {
		cout << "linkchan: << msg ";
		if (verbose & LINKCHAN_DEBUG_DUMP)
			cout << a << endl;
		else
			cout << a.getLen() << endl;
	}
}

const char *linkChan::
getNcpConnectName()
{
	return "LINK.*";
}

void linkChan::
ncpConnectAck()
{
	if (verbose & LINKCHAN_DEBUG_LOG)
		cout << "linkchan: << cack" << endl;
}

void linkChan::
ncpConnectTerminate()
{
	if (verbose & LINKCHAN_DEBUG_LOG)
		cout << "linkchan: << ctrm" << endl;
	terminateWhenAsked();
}
