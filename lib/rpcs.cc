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
#include <stdlib.h>
#include <fstream.h>
#include <iomanip.h>
#include <time.h>
#include <string.h>

#include "defs.h"
#include "bool.h"
#include "rpcs.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "bufferarray.h"

//
// public common API
//
void rpcs::
reconnect()
{
	skt->closeSocket();
	skt->reconnect();
	reset();
}

void rpcs::
reset()
{
	bufferStore a;
	status = E_PSI_FILE_DISC;
	a.addStringT(getConnectName());
	if (skt->sendBufferStore(a)) {
		if (skt->getBufferStore(a) == 1) {
			if (!strcmp(a.getString(0), "Ok"))
				status = E_PSI_GEN_NONE;
		}
	}
}

long rpcs::
getStatus()
{
	return status;
}

const char *rpcs::
getConnectName()
{
	return "SYS$RPCS";
}

//
// protected internals
//
char *rpcs::
convertSlash(const char *name)
{
	char *n = strdup(name);
	for (char *p = n; *p; p++)
		if (*p == '/')
			*p = '\\';
	return n;
}

bool rpcs::
sendCommand(enum commands cc, bufferStore & data)
{
	if (status == E_PSI_FILE_DISC) {
		reconnect();
		if (status == E_PSI_FILE_DISC)
			return false;
	}
	bool result;
	bufferStore a;
	a.addByte(cc);
	a.addBuff(data);
	result = skt->sendBufferStore(a);
	if (!result)
		status = E_PSI_FILE_DISC;
	return result;
}

long rpcs::
getResponse(bufferStore & data)
{
	if (skt->getBufferStore(data) == 1) {
		long ret = data.getByte(0);
		return ret;
	} else
		status = E_PSI_FILE_DISC;
	return status;
}

//
// APIs, identical on SIBO and EPOC
//
int rpcs::
getNCPversion(int &major, int &minor)
{
	bufferStore a;
	if (!sendCommand(QUERY_NCP, a))
		return E_PSI_FILE_DISC;
	long res = getResponse(a);
	a.discardFirstBytes(1);
	if (res)
		return res;
	if (a.getLen() != 2)
		return E_PSI_GEN_FAIL;
	major = a.getByte(0);
	minor = a.getByte(1);
	return res;
}

int rpcs::
execProgram(const char *program, const char *args)
{
	bufferStore a;
	a.addStringT(program);
	int l = strlen(program);
	for (int i = 127; i > l; i--)
		a.addByte(0);
	a.addByte(strlen(args));
	a.addStringT(args);
	if (!sendCommand(EXEC_PROG, a))
		return E_PSI_FILE_DISC;
	long res = getResponse(a);
cout << res << " " << a << endl;
	return res;
}

int rpcs::
stopProgram(const char *program)
{
	bufferStore a;
	a.addStringT(program);
	if (!sendCommand(STOP_PROG, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

int rpcs::
queryProgram(const char *program)
{
	bufferStore a;
	a.addStringT(program);
	if (!sendCommand(QUERY_PROG, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

int rpcs::
formatOpen(const char *drive, int &handle, int &count)
{
	bufferStore a;
	a.addStringT(drive);
	if (!sendCommand(FORMAT_OPEN, a))
		return E_PSI_FILE_DISC;
	long res = getResponse(a);
	a.discardFirstBytes(1);
	if (res)
		return res;
	if (a.getLen() != 4)
		return E_PSI_GEN_FAIL;
	handle = a.getWord(0);
	count = a.getWord(2);
	return res;
}

int rpcs::
formatRead(int handle)
{
	bufferStore a;
	a.addWord(handle);
	if (!sendCommand(FORMAT_READ, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

int rpcs::
getUniqueID(const char *device, long &id)
{
	bufferStore a;
	a.addStringT(device);
	if (!sendCommand(GET_UNIQUEID, a))
		return E_PSI_FILE_DISC;
	long res = getResponse(a);
	a.discardFirstBytes(1);
	if (res)
		return res;
	if (a.getLen() != 4)
		return E_PSI_GEN_FAIL;
	id = a.getDWord(0);
	return res;
}

int rpcs::
getOwnerInfo(bufferArray &ret)
{
	bufferStore a;
	if (!sendCommand(GET_OWNERINFO, a))
		return E_PSI_FILE_DISC;
	long res = getResponse(a);
	a.discardFirstBytes(1);
	if (res)
		return res;
	a.addByte(0);
	int l = a.getLen();
	char *s = (char *)a.getString(0);
	for (int i = 0; i < l; i++)
		if (s[i] == 6)
			s[i] = 0;
	ret.clear();
	while (l > 0) {
		bufferStore b;
		b.addStringT(s);
		ret += b;
		l -= (strlen(s) + 1);
		s += (strlen(s) + 1);
	}
	return res;
}

int rpcs::
getMachineType(int &type)
{
	bufferStore a;
	if (!sendCommand(GET_MACHINETYPE, a))
		return E_PSI_FILE_DISC;
	long res = getResponse(a);
	a.discardFirstBytes(1);
	if (res)
		return res;
	if (a.getLen() != 2)
		return E_PSI_GEN_FAIL;
	type = a.getWord(0);
	return res;
}

int rpcs::
fuser(const char *name, char *buf, int bufsize)
{
	bufferStore a;
	a.addStringT(name);
	if (!sendCommand(FUSER, a))
		return E_PSI_FILE_DISC;
	long res = getResponse(a);
	a.discardFirstBytes(1);
	if (res)
		return res;
	int len = ((int)a.getLen() > bufsize) ? bufsize - 1 : a.getLen();
	strncpy(buf, a.getString(0), len);
	buf[len] = '\0';
	char *p;
	while ((p = strchr(buf, 6)))
		*p = '\n';
	return res;
}

int rpcs::
quitServer()
{
	bufferStore a;
	if (!sendCommand(QUIT_SERVER, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

