//
//  PLP - An implementation of the PSION link protocol
//
//  Copyright (C) 1999  Philip Proudman
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
#include <stdlib.h>
#include <fstream.h>
#include <iomanip.h>
#include <time.h>
#include <string.h>

#include "defs.h"
#include "bool.h"
#include "rfsv32.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "bufferarray.h"

#define RFSV_SENDLEN 230

rfsv32::rfsv32(ppsocket * _skt) : serNum(0)
{
	skt = _skt;
	reset();
}

rfsv32::~rfsv32()
{
	bufferStore a;
	a.addStringT("Close");
	if (status == PSI_ERR_NONE)
		skt->sendBufferStore(a);
	skt->closeSocket();
}

void rfsv32::
reconnect()
{
	skt->closeSocket();
	skt->reconnect();
	serNum = 0;
	reset();
}

void rfsv32::
reset()
{
	bufferStore a;
	status = PSI_ERR_DISCONNECTED;
	a.addStringT(getConnectName());
	if (skt->sendBufferStore(a)) {
		if (skt->getBufferStore(a) == 1) {
			if (!strcmp(a.getString(0), "Ok"))
				status = PSI_ERR_NONE;
		}
	}
}

long rfsv32::
getStatus()
{
	return status;
}

const char *rfsv32::
getConnectName()
{
	return "SYS$RFSV.*";
}

void rfsv32::
convertSlash(const char *name)
{
	for (char *p = (char *)name; *p; p++)
		if (*p == '/')
			*p = '\\';
}

long rfsv32::
fopen(long attr, const char *name, long &handle)
{
	bufferStore a;
	a.addDWord(attr);
	convertSlash(name);
	a.addWord(strlen(name));
	a.addString(name);
	if (!sendCommand(OPEN_FILE, a))
		return PSI_ERR_DISCONNECTED;
	long res = getResponse(a);
	if (!res && a.getLen() == 4) {
		handle = a.getDWord(0);
		return 0;
	}
	return res;
}

long rfsv32::
mktemp(long *handle, char *tmpname)
{
	bufferStore a;
	if (!sendCommand(TEMP_FILE, a))
		return PSI_ERR_DISCONNECTED;
	long res = getResponse(a);
	if (res == 0) {
		*handle = a.getDWord(0);
		strcpy(tmpname, a.getString(6));
	}
	return res;
}

long rfsv32::
fcreatefile(long attr, const char *name, long &handle)
{
	bufferStore a;
	a.addDWord(attr);
	convertSlash(name);
	a.addWord(strlen(name));
	a.addString(name);
	if (!sendCommand(CREATE_FILE, a))
		return PSI_ERR_DISCONNECTED;
	long res = getResponse(a);
	if (!res && a.getLen() == 4)
		handle = a.getDWord(0);
	return res;
}

long rfsv32::
freplacefile(long attr, const char *name, long &handle)
{
	bufferStore a;
	convertSlash(name);
	a.addDWord(attr);
	a.addWord(strlen(name));
	a.addString(name);
	if (!sendCommand(REPLACE_FILE, a))
		return PSI_ERR_DISCONNECTED;
	long res = getResponse(a);
	if (!res && a.getLen() == 4)
		handle = a.getDWord(0);
	return res;
}

long rfsv32::
fopendir(long attr, const char *name, long &handle)
{
	bufferStore a;
	convertSlash(name);
	a.addDWord(attr);
	a.addWord(strlen(name));
	a.addString(name);
	if (!sendCommand(OPEN_DIR, a))
		return PSI_ERR_DISCONNECTED;
	long res = getResponse(a);
	if (!res && a.getLen() == 4)
		handle = a.getDWord(0);
	return res;
}

long rfsv32::
fclose(long handle)
{
	bufferStore a;
	a.addDWord(handle);
	if (!sendCommand(CLOSE_HANDLE, a))
		return PSI_ERR_DISCONNECTED;
	return getResponse(a);
}

/* Microseconds since 1.1.1980 00:00:00 */
#define PSI_EPOCH_SECS8 0x0e8c52f4
#define EPOCH_2H  7200
#define EPOCH_DIFF_SECS (3652 * 24 * 60 * 60)

unsigned long rfsv32::
micro2time(unsigned long microHi, unsigned long microLo)
{
	unsigned long long micro = microHi;
	unsigned long long pes = PSI_EPOCH_SECS8;
	pes <<= 8;
	micro <<= 32;
	micro += microLo;
	micro /= 1000000;
	micro -= pes;
	micro += EPOCH_DIFF_SECS;
	micro -= EPOCH_2H;
	return (long) micro;
}

void rfsv32::
time2micro(unsigned long time, unsigned long &microHi, unsigned long &microLo)
{
	unsigned long long micro = (unsigned long long)time;
	unsigned long long pes = PSI_EPOCH_SECS8;
	pes <<= 8;
	micro += pes;
	micro -= EPOCH_DIFF_SECS;
	micro += EPOCH_2H;
	micro *= (unsigned long long)1000000;
	microLo = (micro & (unsigned long long)0x0FFFFFFFF);
	micro >>= 32;
	microHi = (micro & (unsigned long long)0x0FFFFFFFF);
}

long rfsv32::
dir(const char *name, bufferArray * files)
{
	long handle;
	long res = fopendir(PSI_ATTR_HIDDEN | PSI_ATTR_SYSTEM | PSI_ATTR_DIRECTORY, name, handle);
	if (res != 0)
		return res;

	while (1) {
		bufferStore a;
		a.addDWord(handle);
		if (!sendCommand(READ_DIR, a))
			return PSI_ERR_DISCONNECTED;
		res = getResponse(a);
		if (res)
			break;
		while (a.getLen() > 16) {
			long shortLen = a.getDWord(0);
			long attributes = a.getDWord(4);
			long size = a.getDWord(8);
			unsigned long modLow = a.getDWord(12);
			unsigned long modHi = a.getDWord(16);
			// long uid1 = a.getDWord(20);
			// long uid2 = a.getDWord(24);
			// long uid3 = a.getDWord(28);
			long longLen = a.getDWord(32);

			long date = micro2time(modHi, modLow);

			bufferStore s;
			s.addDWord(date);
			s.addDWord(size);
			s.addDWord(attributes);
			int d = 36;
			for (int i = 0; i < longLen; i++, d++)
				s.addByte(a.getByte(d));
			s.addByte(0);
			while (d % 4)
				d++;
			files->pushBuffer(s);
			d += shortLen;
			while (d % 4)
				d++;
			a.discardFirstBytes(d);
		}
	}
	if (res == PSI_ERR_EoF)
		res = 0;
	fclose(handle);
	return res;
}

long rfsv32::
fgetmtime(const char *name, long *mtime)
{
	bufferStore a;
	convertSlash(name);
	a.addWord(strlen(name));
	a.addString(name);
	if (!sendCommand(MODIFIED, a))
		return PSI_ERR_DISCONNECTED;
	long res = getResponse(a);
	if (res != 0)
		return res;
	*mtime = micro2time(a.getDWord(4), a.getDWord(0));
	return 0;
}

long rfsv32::
fsetmtime(const char *name, long mtime)
{
	bufferStore a;
	unsigned long microLo, microHi;
	time2micro(mtime, microHi, microLo);
	convertSlash(name);
	a.addDWord(microLo);
	a.addDWord(microHi);
	a.addWord(strlen(name));
	a.addString(name);
	if (!sendCommand(SET_MODIFIED, a))
		return PSI_ERR_DISCONNECTED;
	long res = getResponse(a);
	if (res != 0)
		return res;
	return 0;
}

long rfsv32::
fgetattr(const char *name, long *attr)
{
	bufferStore a;
	convertSlash(name);
	a.addWord(strlen(name));
	a.addString(name);
	if (!sendCommand(ATT, a))
		return PSI_ERR_DISCONNECTED;
	long res = getResponse(a);
	if (res != 0)
		return res;
	*attr = a.getDWord(0);
	return 0;
}

long rfsv32::
fgeteattr(const char *name, long *attr, long *size, long *time)
{
	bufferStore a;
	convertSlash(name);
	a.addWord(strlen(name));
	a.addString(name);
	if (!sendCommand(REMOTE_ENTRY, a))
		return PSI_ERR_DISCONNECTED;
	long res = getResponse(a);
	if (res != 0)
		return res;
	// long shortLen = a.getDWord(0);
	*attr = a.getDWord(4);
	*size = a.getDWord(8);
	unsigned long modLow = a.getDWord(12);
	unsigned long modHi = a.getDWord(16);
	// long uid1 = a.getDWord(20);
	// long uid2 = a.getDWord(24);
	// long uid3 = a.getDWord(28);
	// long longLen = a.getDWord(32);
	*time = micro2time(modHi, modLow);
	return 0;
}

long rfsv32::
fsetattr(const char *name, long seta, long unseta)
{
	bufferStore a;
	convertSlash(name);
	a.addDWord(seta);
	a.addDWord(unseta);
	a.addWord(strlen(name));
	a.addString(name);
	if (!sendCommand(SET_ATT, a))
		return PSI_ERR_DISCONNECTED;
	return getResponse(a);
}

long rfsv32::
dircount(const char *name, long *count)
{
	long handle;
	convertSlash(name);
	long res = fopendir(PSI_ATTR_HIDDEN | PSI_ATTR_SYSTEM | PSI_ATTR_DIRECTORY, name, handle);
	*count = 0;
	if (res != 0)
		return res;

	while (1) {
		bufferStore a;
		a.addDWord(handle);
		if (!sendCommand(READ_DIR, a))
			return PSI_ERR_DISCONNECTED;
		res = getResponse(a);
		if (res)
			break;
		while (a.getLen() > 16) {
			int d = 36 + a.getDWord(32);
			while (d % 4)
				d++;
			d += a.getDWord(0);
			while (d % 4)
				d++;
			a.discardFirstBytes(d);
			(*count)++;
		}
	}
	fclose(handle);
	if (res == PSI_ERR_EoF)
		res = 0;
	return res;
}

long rfsv32::
devlist(long *devbits)
{
	bufferStore a;
	long res;

	if (!sendCommand(GET_DRIVE_LIST, a))
		return PSI_ERR_DISCONNECTED;
	res = getResponse(a);
	*devbits = 0;
	if ((res == 0) && (a.getLen() == 26)) {
		for (int i = 25; i >= 0; i--) {
			*devbits <<= 1;
			if (a.getByte(i) != 0)
				*devbits |= 1;
		}
	}
	return res;
}

char *rfsv32::
devinfo(int devnum, long *vfree, long *vtotal, long *vattr,
	long *vuniqueid)
{
	bufferStore a;
	long res;

	a.addDWord(devnum);
	if (!sendCommand(DRIVE_INFO, a))
		return NULL;
	res = getResponse(a);
	if (res == 0) {
		*vattr = a.getDWord(0);
		*vuniqueid = a.getDWord(16);
		*vtotal = a.getDWord(20);
		*vfree = a.getDWord(28);
		// vnamelen = a.getDWord(36);
		a.addByte(0);
		return (strdup(a.getString(40)));
	}
	return NULL;
}

bool rfsv32::
sendCommand(enum commands cc, bufferStore & data)
{
	if (status == PSI_ERR_DISCONNECTED) {
		reconnect();
		if (status == PSI_ERR_DISCONNECTED)
			return false;
	}
	bool result;
	bufferStore a;
	a.addWord(cc);
	a.addWord(serNum);
	if (serNum < 0xffff)
		serNum++;
	else
		serNum = 0;
	a.addBuff(data);
	result = skt->sendBufferStore(a);
	if (!result)
		status = PSI_ERR_DISCONNECTED;
	return result;
}

long rfsv32::
getResponse(bufferStore & data)
{
	if (skt->getBufferStore(data) == 1 &&
	    data.getWord(0) == 0x11) {
		long ret = data.getDWord(4);
		data.discardFirstBytes(8);
		return ret;
	} else
		status = PSI_ERR_DISCONNECTED;
	return status;
}

char * rfsv32::
opErr(long status)
{
	enum errs e = (enum errs) status;
	switch (e) {
		case PSI_ERR_NONE:
			return "";
		case PSI_ERR_NOT_FOUND:
			return "not found";
		case PSI_ERR_GENERAL:
			return "general";
			break;
		case PSI_ERR_CANCEL:
			return "cancelled";
			break;
		case PSI_ERR_NO_MEMORY:
			return "out of memory";
			break;
		case PSI_ERR_NOT_SUPPORTED:
			return "unsupported";
			break;
		case PSI_ERR_ARGUMENT:
			return "bad argument";
			break;
		case PSI_ERR_TOTAL_LOSS_OF_PRECISION:
			return "total loss of precision";
			break;
		case PSI_ERR_BAD_HANDLE:
			return "bad handle";
			break;
		case PSI_ERR_OVERFLOW:
			return "overflow";
			break;
		case PSI_ERR_UNDERFLOW:
			return "underflow";
			break;
		case PSI_ERR_ALREADY_EXISTS:
			return "file already exists";
			break;
		case PSI_ERR_PATH_NOT_FOUND:
			return "path not found";
			break;
		case PSI_ERR_DIED:
			return "DIED";
			break;
		case PSI_ERR_IN_USE:
			return "resource in use";
			break;
		case PSI_ERR_SERVER_TERMINATED:
			return "server terminated";
			break;
		case PSI_ERR_SERVER_BUSY:
			return "server busy";
			break;
		case PSI_ERR_COMPLETION:
			return "completed";
			break;
		case PSI_ERR_NOT_READY:
			return "not ready";
			break;
		case PSI_ERR_UNKNOWN:
			return "unknown";
			break;
		case PSI_ERR_CORRUPT:
			return "corrupt";
			break;
		case PSI_ERR_ACCESS_DENIED:
			return "permission denied";
			break;
		case PSI_ERR_LOCKED:
			return "resource locked";
			break;
		case PSI_ERR_WRITE:
			return "write";
			break;
		case PSI_ERR_DISMOUNTED:
			return "dismounted";
			break;
		case PSI_ERR_EoF:
			return "end of file";
			break;
		case PSI_ERR_DISK_FULL:
			return "disk full";
			break;
		case PSI_ERR_BAD_DRIVER:
			return "bad driver";
			break;
		case PSI_ERR_BAD_NAME:
			return "bad name";
			break;
		case PSI_ERR_COMMS_LINE_FAIL:
			return "comms line failed";
			break;
		case PSI_ERR_COMMS_FRAME:
			return "comms framing error";
			break;
		case PSI_ERR_COMMS_OVERRUN:
			return "comms overrun";
			break;
		case PSI_ERR_COMMS_PARITY:
			return "comms parity error";
			break;
		case PSI_ERR_TIMEOUT:
			return "timed out";
			break;
		case PSI_ERR_COULD_NOT_CONNECT:
			return "could not connect";
			break;
		case PSI_ERR_COULD_NOT_DISCONNECT:
			return "could not disconnect";
			break;
		case PSI_ERR_DISCONNECTED:
			return "unit disconnected";
			break;
		case PSI_ERR_BAD_LIBRARY_ENTRY_POINT:
			return "bad library entry point";
			break;
		case PSI_ERR_BAD_DESCRIPTOR:
			return "bad descriptor";
			break;
		case PSI_ERR_ABORT:
			return "abort";
			break;
		case PSI_ERR_TOO_BIG:
			return "too big";
			break;
		case PSI_ERR_DIVIDE_BY_ZERO:
			return "division by zero";
			break;
		case PSI_ERR_BAD_POWER:
			return "bad power";
			break;
		case PSI_ERR_DIR_FULL:
			return "directory full";
			break;
		default:
			return "Unknown error";
			break;
	}
}

long rfsv32::
fread(long handle, char *buf, long len)
{
	long res;
	long count = 0;

	do {
		bufferStore a;
		a.addDWord(handle);
		a.addDWord(((len-count) > 2000)?2000:(len-count));
		if (!sendCommand(READ_FILE, a))
			return PSI_ERR_DISCONNECTED;
		if ((res = getResponse(a)) != 0)
			return res;
		res = a.getLen();
		if (res > 0) {
			memcpy(buf, a.getString(), res);
			count += res;
			buf += res;
		}
	} while ((count < len) && (res > 0));
	return (res < 0)?res:count;
}

long rfsv32::
fwrite(long handle, char *buf, long len)
{
	long res;
	long total = 0;
	long count;

	do {
		count = ((len - total) > 2000)?2000:(len - total); 
		bufferStore a;
		bufferStore tmp((unsigned char *)buf, count);
		a.addDWord(handle);
		a.addBuff(tmp);
		if (!sendCommand(WRITE_FILE, a))
			return PSI_ERR_DISCONNECTED;
		if ((res = getResponse(a)) != 0)
			return res;
		total += count;
		buf += count;
	} while ((total < len) && (count > 0));
	return total;
}

long rfsv32::
copyFromPsion(const char *from, const char *to)
{
	long handle;
	long res;
	long len;

	if ((res = fopen(PSI_OMODE_SHARE_READERS | PSI_OMODE_BINARY, from, handle)) != 0)
		return res;
	ofstream op(to);
	if (!op) {
		fclose(handle);
		return -1;
	}
	do {
		char buf[2000];
		if ((len = fread(handle, buf, sizeof(buf))) > 0)
			op.write(buf, len);
	} while (len > 0);

	fclose(handle);
	op.close();
	return len;
}

long rfsv32::
copyToPsion(const char *from, const char *to)
{
	long handle;
	long res;

	ifstream ip(from);
	if (!ip)
		return PSI_ERR_NOT_FOUND;
	res = fcreatefile(PSI_OMODE_BINARY | PSI_OMODE_SHARE_EXCLUSIVE | PSI_OMODE_READ_WRITE, to, handle);
	if (res != 0) {
		res = freplacefile(PSI_OMODE_BINARY | PSI_OMODE_SHARE_EXCLUSIVE | PSI_OMODE_READ_WRITE, to, handle);
		if (res != 0)
			return res;
	}
	unsigned char *buff = new unsigned char[RFSV_SENDLEN];
	int total = 0;
	while (ip && !ip.eof()) {
		ip.read(buff, RFSV_SENDLEN);
		bufferStore tmp(buff, ip.gcount());
		total += tmp.getLen();
		if (tmp.getLen() == 0)
			break;
		bufferStore a;
		a.addDWord(handle);
		a.addBuff(tmp);
		if (!sendCommand(WRITE_FILE, a))
			return PSI_ERR_DISCONNECTED;
		res = getResponse(a);
		if (res) {
			fclose(handle);
			return res;
		}
	}
	fclose(handle);
	ip.close();
	delete[]buff;
	return 0;
}

long rfsv32::
fsetsize(long handle, long size)
{
	bufferStore a;
	a.addDWord(handle);
	a.addDWord(size);
	if (!sendCommand(SET_SIZE, a))
		return PSI_ERR_DISCONNECTED;
	return getResponse(a);
}

/*
 * Unix-like implementation off fseek with one
 * exception: If seeking beyond eof, the gap
 * contains garbage instead of zeroes.
 */
long rfsv32::
fseek(long handle, long pos, long mode)
{
	bufferStore a;
	long res;
	long savpos = 0;
	long realpos;
	long calcpos = 0;

/*
   seek-parameter for psion:
   dword position
   dword handle
   dword mode
   1 = from start
   2 = from current pos
   3 = from end
   ??no more?? 4 = sense recpos
   ??no more?? 5 = set recpos
   ??no more?? 6 = text-rewind
 */

	if ((mode < PSI_SEEK_SET) || (mode > PSI_SEEK_END))
		return PSI_ERR_ARGUMENT;

	if ((mode == PSI_SEEK_CUR) && (pos >= 0)) {
		/* get and save current position */
		a.addDWord(0);
		a.addDWord(handle);
		a.addDWord(PSI_SEEK_CUR);
		if (!sendCommand(SEEK_FILE, a))
			return PSI_ERR_DISCONNECTED;
		if ((res = getResponse(a)) != 0)
			return res;
		savpos = a.getDWord(0);
		if (pos == 0)
			return savpos;
		a.init();
	}
	if ((mode == PSI_SEEK_END) && (pos >= 0)) {
		/* get and save end position */
		a.addDWord(0);
		a.addDWord(handle);
		a.addDWord(PSI_SEEK_END);
		if (!sendCommand(SEEK_FILE, a))
			return PSI_ERR_DISCONNECTED;
		if ((res = getResponse(a)) != 0)
			return res;
		savpos = a.getDWord(0);
		if (pos == 0)
			return savpos;
		/* Expand file */
		a.init();
		a.addDWord(handle);
		a.addDWord(savpos + pos);
		if (!sendCommand(SET_SIZE, a))
			return PSI_ERR_DISCONNECTED;
		if ((res = getResponse(a)) != 0)
			return res;
		pos = 0;
		a.init();
	}
	/* Now the real seek */
	a.addDWord(pos);
	a.addDWord(handle);
	a.addDWord(mode);
	if (!sendCommand(SEEK_FILE, a))
		return PSI_ERR_DISCONNECTED;
	if ((res = getResponse(a)) != 0)
		return res;
	realpos = a.getDWord(0);
	switch (mode) {
		case PSI_SEEK_SET:
			calcpos = pos;
			break;
		case PSI_SEEK_CUR:
			calcpos = savpos + pos;
			break;
		case PSI_SEEK_END:
			return realpos;
			break;
	}
	if (calcpos > realpos) {
		/* Beyond end of file */
		a.init();
		a.addDWord(handle);
		a.addDWord(calcpos);
		if (!sendCommand(SET_SIZE, a))
			return PSI_ERR_DISCONNECTED;
		if ((res = getResponse(a)) != 0)
			return res;
		a.addDWord(calcpos);
		a.addDWord(handle);
		a.addDWord(PSI_SEEK_SET);
		if (!sendCommand(SEEK_FILE, a))
			return PSI_ERR_DISCONNECTED;
		if ((res = getResponse(a)) != 0)
			return res;
		realpos = a.getDWord(0);
	}
	return realpos;
}

long rfsv32::
mkdir(const char *name)
{
	bufferStore a;
	convertSlash(name);
	if (strlen(name) && (name[strlen(name) - 1] != '\\')) {
		a.addWord(strlen(name) + 1);
		a.addString(name);
		a.addByte('\\');
	} else {
		a.addWord(strlen(name));
		a.addString(name);
	}
	if (!sendCommand(MK_DIR_ALL, a))
		return PSI_ERR_DISCONNECTED;
	return getResponse(a);
}

long rfsv32::
rmdir(const char *name)
{
	bufferStore a;
	convertSlash(name);
	if (strlen(name) && (name[strlen(name) - 1] != '\\')) {
		a.addWord(strlen(name) + 1);
		a.addString(name);
		a.addByte('\\');
	} else {
		a.addWord(strlen(name));
		a.addString(name);
	}
	if (!sendCommand(RM_DIR, a))
		return PSI_ERR_DISCONNECTED;
	return getResponse(a);
}

long rfsv32::
rename(const char *oldname, const char *newname)
{
	bufferStore a;
	convertSlash(oldname);
	convertSlash(newname);
	a.addWord(strlen(oldname));
	a.addString(oldname);
	a.addWord(strlen(newname));
	a.addString(newname);
	if (!sendCommand(RENAME, a))
		return PSI_ERR_DISCONNECTED;
	return getResponse(a);
}

long rfsv32::
remove(const char *name)
{
	bufferStore a;
	convertSlash(name);
	a.addWord(strlen(name));
	a.addString(name);
	if (!sendCommand(DELETE, a))
		return PSI_ERR_DISCONNECTED;
	return getResponse(a);
}
