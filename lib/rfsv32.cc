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

#include "bool.h"
#include "bool.h"
#include "rfsv32.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "../defaults.h"
#include "bufferarray.h"

rfsv32::rfsv32(ppsocket * _skt) : serNum(0)
{
	skt = _skt;
	bufferStore a;
	status = DISCONNECTED;
	a.addStringT(getConnectName());
	if (skt->sendBufferStore(a)) {
		if (skt->getBufferStore(a) == 1) {
			if (strcmp(a.getString(0), "Ok"))
				cerr << "Not got ok over socket\n";
			else
				status = NONE;
		}
	}
}

rfsv32::~rfsv32()
{
	bufferStore a;
	a.addStringT("Close");
	if (status == NONE)
		skt->sendBufferStore(a);
	skt->closeSocket();
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
	sendCommand(OPEN_FILE, a);

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
	sendCommand(TEMP_FILE, a);
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
	sendCommand(CREATE_FILE, a);
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
	sendCommand(REPLACE_FILE, a);
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
	sendCommand(OPEN_DIR, a);
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
	sendCommand(CLOSE_HANDLE, a);
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
	long res = fopendir(HIDDEN | SYSTEM | DIRECTORY, name, handle);
	if (res != 0)
		return res;

	while (1) {
		bufferStore a;
		a.addDWord(handle);
		sendCommand(READ_DIR, a);
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
			if (!files) {
				char dateBuff[100];
				struct tm *t;
				t = localtime(&date);
				strftime(dateBuff, 100, "%c", t);
				cout << ((attributes & DIRECTORY) ? "d" : "-");
				cout << ((attributes & READ_ONLY) ? "-" : "w");
				cout << ((attributes & HIDDEN) ? "h" : "-");
				cout << ((attributes & SYSTEM) ? "s" : "-");
				cout << ((attributes & ARCHIVE) ? "a" : "-");
				cout << ((attributes & VOLUME) ? "v" : "-");
				cout << ((attributes & NORMAL) ? "n" : "-");
				cout << ((attributes & TEMPORARY) ? "t" : "-");
				cout << ((attributes & COMPRESSED) ? "c" : "-");
				cout << " " << dec << setw(10) << setfill(' ') << size;
				cout << " " << dateBuff;
			} else {
				s.addDWord(date);
				s.addDWord(size);
				s.addDWord(attributes);
			}
			int d = 36;
			for (int i = 0; i < longLen; i++, d++)
				s.addByte(a.getByte(d));
			s.addByte(0);
			while (d % 4)
				d++;
			if (!files)
				cout << " " << s.getString() << endl;
			else /* if ((attributes & DIRECTORY) == 0) */
				files->pushBuffer(s);
			d += shortLen;
			while (d % 4)
				d++;
			a.discardFirstBytes(d);
		}
	}
	if (res == EoF)
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
	sendCommand(MODIFIED, a);
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
	sendCommand(SET_MODIFIED, a);
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
	sendCommand(ATT, a);
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
	sendCommand(REMOTE_ENTRY, a);
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
	sendCommand(SET_ATT, a);
	return getResponse(a);
}

long rfsv32::
dircount(const char *name, long *count)
{
	long handle;
	convertSlash(name);
	long res = fopendir(HIDDEN | SYSTEM | DIRECTORY, name, handle);
	*count = 0;
	if (res != 0)
		return res;

	while (1) {
		bufferStore a;
		a.addDWord(handle);
		sendCommand(READ_DIR, a);
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
	if (res == EoF)
		res = 0;
	return res;
}

long rfsv32::
devlist(long *devbits)
{
	bufferStore a;
	long res;

	sendCommand(GET_DRIVE_LIST, a);
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
	sendCommand(DRIVE_INFO, a);
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
	bufferStore a;
	a.addWord(cc);
	a.addWord(serNum);
	if (serNum < 0xffff)
		serNum++;
	else
		serNum = 0;
	a.addBuff(data);
	return skt->sendBufferStore(a);
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
		status = DISCONNECTED;
	return (enum errs) GENERAL;
}

char * rfsv32::
opErr(long status)
{
	enum errs e = (enum errs) status;
	switch (e) {
		case NONE:
			return "";
		case NOT_FOUND:
			return "NOT_FOUND";
		case GENERAL:
			return "GENERAL";
			break;
		case CANCEL:
			return "CANCEL";
			break;
		case NO_MEMORY:
			return "NO_MEMORY";
			break;
		case NOT_SUPPORTED:
			return "NOT_SUPPORTED";
			break;
		case ARGUMENT:
			return "ARGUMENT";
			break;
		case TOTAL_LOSS_OF_PRECISION:
			return "TOTAL_LOSS_OF_PRECISION";
			break;
		case BAD_HANDLE:
			return "BAD_HANDLE";
			break;
		case OVERFLOW:
			return "OVERFLOW";
			break;
		case UNDERFLOW:
			return "UNDERFLOW";
			break;
		case ALREADY_EXISTS:
			return "ALREADY_EXISTS";
			break;
		case PATH_NOT_FOUND:
			return "PATH_NOT_FOUND";
			break;
		case DIED:
			return "DIED";
			break;
		case IN_USE:
			return "IN_USE";
			break;
		case SERVER_TERMINATED:
			return "SERVER_TERMINATED";
			break;
		case SERVER_BUSY:
			return "SERVER_BUSY";
			break;
		case COMPLETION:
			return "COMPLETION";
			break;
		case NOT_READY:
			return "NOT_READY";
			break;
		case UNKNOWN:
			return "UNKNOWN";
			break;
		case CORRUPT:
			return "CORRUPT";
			break;
		case ACCESS_DENIED:
			return "ACCESS_DENIED";
			break;
		case LOCKED:
			return "LOCKED";
			break;
		case WRITE:
			return "WRITE";
			break;
		case DISMOUNTED:
			return "DISMOUNTED";
			break;
		case EoF:
			return "EOF";
			break;
		case DISK_FULL:
			return "DISK_FULL";
			break;
		case BAD_DRIVER:
			return "BAD_DRIVER";
			break;
		case BAD_NAME:
			return "BAD_NAME";
			break;
		case COMMS_LINE_FAIL:
			return "COMMS_LINE_FAIL";
			break;
		case COMMS_FRAME:
			return "COMMS_FRAME";
			break;
		case COMMS_OVERRUN:
			return "COMMS_OVERRUN";
			break;
		case COMMS_PARITY:
			return "COMMS_PARITY";
			break;
		case PSI_TIMEOUT:
			return "TIMEOUT";
			break;
		case COULD_NOT_CONNECT:
			return "COULD_NOT_CONNECT";
			break;
		case COULD_NOT_DISCONNECT:
			return "COULD_NOT_DISCONNECT";
			break;
		case DISCONNECTED:
			return "DISCONNECTED";
			break;
		case BAD_LIBRARY_ENTRY_POINT:
			return "BAD_LIBRARY_ENTRY_POINT";
			break;
		case BAD_DESCRIPTOR:
			return "BAD_DESCRIPTOR";
			break;
		case ABORT:
			return "ABORT";
			break;
		case TOO_BIG:
			return "TOO_BIG";
			break;
		case DIVIDE_BY_ZERO:
			return "DIVIDE_BY_ZERO";
			break;
		case BAD_POWER:
			return "BAD_POWER";
			break;
		case DIR_FULL:
			return "DIR_FULL";
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
		sendCommand(READ_FILE, a);
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
		sendCommand(WRITE_FILE, a);
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

	if ((res = fopen(SHARE_READERS | BINARY, from, handle)) != 0)
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
		return NOT_FOUND;
	res = fcreatefile(BINARY | SHARE_EXCLUSIVE | READ_WRITE, to, handle);
	if (res != 0) {
		res = freplacefile(BINARY | SHARE_EXCLUSIVE | READ_WRITE, to, handle);
		if (res != 0) {
			opErr(res);
			cerr << endl;
			return res;
		}
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
		sendCommand(WRITE_FILE, a);
		res = getResponse(a);
		if (res) {
			cerr << "Unknown response to fwrite - ";
			opErr(res);
			cerr << " " << a << endl;
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
	sendCommand(SET_SIZE, a);
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

	if ((mode < PSEEK_SET) || (mode > PSEEK_END))
		return ARGUMENT;

	if ((mode == PSEEK_CUR) && (pos >= 0)) {
		/* get and save current position */
		a.addDWord(0);
		a.addDWord(handle);
		a.addDWord(PSEEK_CUR);
		sendCommand(SEEK_FILE, a);
		if ((res = getResponse(a)) != 0)
			return res;
		savpos = a.getDWord(0);
		if (pos == 0)
			return savpos;
		a.init();
	}
	if ((mode == PSEEK_END) && (pos >= 0)) {
		/* get and save end position */
		a.addDWord(0);
		a.addDWord(handle);
		a.addDWord(PSEEK_END);
		sendCommand(SEEK_FILE, a);
		if ((res = getResponse(a)) != 0)
			return res;
		savpos = a.getDWord(0);
		if (pos == 0)
			return savpos;
		/* Expand file */
		a.init();
		a.addDWord(handle);
		a.addDWord(savpos + pos);
		sendCommand(SET_SIZE, a);
		if ((res = getResponse(a)) != 0)
			return res;
		pos = 0;
		a.init();
	}
	/* Now the real seek */
	a.addDWord(pos);
	a.addDWord(handle);
	a.addDWord(mode);
	sendCommand(SEEK_FILE, a);
	if ((res = getResponse(a)) != 0) {
cout << "seekRES(" << handle << ")=" << res << endl;
		return res;
}
	realpos = a.getDWord(0);
cout << "seekPOS=" << realpos << endl;
	switch (mode) {
		case PSEEK_SET:
			calcpos = pos;
			break;
		case PSEEK_CUR:
			calcpos = savpos + pos;
			break;
		case PSEEK_END:
			return realpos;
			break;
	}
	if (calcpos > realpos) {
		/* Beyond end of file */
		a.init();
		a.addDWord(handle);
		a.addDWord(calcpos);
		sendCommand(SET_SIZE, a);
		if ((res = getResponse(a)) != 0)
			return res;
		a.addDWord(calcpos);
		a.addDWord(handle);
		a.addDWord(PSEEK_SET);
		sendCommand(SEEK_FILE, a);
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
	sendCommand(MK_DIR_ALL, a);
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
	sendCommand(RM_DIR, a);
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
	sendCommand(RENAME, a);
	return getResponse(a);
}

long rfsv32::
remove(const char *name)
{
	bufferStore a;
	convertSlash(name);
	a.addWord(strlen(name));
	a.addString(name);
	sendCommand(DELETE, a);
	return getResponse(a);
}
