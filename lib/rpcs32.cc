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

#include <stream.h>
#include <stdio.h>
#include <stdlib.h>
#include <fstream.h>
#include <iomanip.h>
#include <time.h>
#include <string.h>

#include "defs.h"
#include "bool.h"
#include "rpcs32.h"
#include "bufferstore.h"
#include "bufferarray.h"
#include "ppsocket.h"

rpcs32::rpcs32(ppsocket * _skt)
{
	skt = _skt;
	reset();
}

rpcs32::~rpcs32()
{
	bufferStore a;
	a.addStringT("Close");
	if (status == E_PSI_GEN_NONE)
		skt->sendBufferStore(a);
	skt->closeSocket();
}

int rpcs32::
queryDrive(char drive, bufferArray &ret)
{
	bufferStore a;
	a.addByte(drive);
	if (!sendCommand(rpcs::QUERY_DRIVE, a))
		return rpcs::E_PSI_FILE_DISC;
	getResponse(a);
	int l = a.getLen();
	ret.clear();
//cout << dec << "qd: " << a.getLen() << " a="<< a << endl;
	while (l > 1) {
		bufferStore b, c;
		const char *s;
		char *p;
		int pid;
		int sl;

		s = a.getString(0);
		sl = strlen(s) + 1;
		l -= sl;
		a.discardFirstBytes(sl);
		p = strstr(s, ".$");
		if (p) {
			*p = '\0'; p += 2;
			sscanf(p, "%d", &pid);
		} else
			pid = 0;
		b.addWord(pid);
		b.addStringT(s);
		s = a.getString(0);
		sl = strlen(s) + 1;
		l -= sl;
		a.discardFirstBytes(sl);
		c.addStringT(s);
		ret.push(c);
		ret.push(b);
	}
	return 0;
}

int rpcs32::
getCmdLine(const char *process, char *buf, int bufsize)
{
	return 0;
}

int rpcs32::
configOpen(void)
{
	bufferStore a;
cout << "configOpen:" << endl;
	if (!sendCommand(rpcs::CONFIG_OPEN, a))
		return rpcs::E_PSI_FILE_DISC;
	getResponse(a);
cout << a << endl;
}

int rpcs32::
configRead(void)
{
	bufferStore a;
cout << "configRead:" << endl;
	if (!sendCommand(rpcs::CONFIG_READ, a))
		return rpcs::E_PSI_FILE_DISC;
	getResponse(a);
cout << a << endl;
}
