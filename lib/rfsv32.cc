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

rfsv32::rfsv32(ppsocket * _skt) : serNum(0)
{
	skt = _skt;
	reset();
}

rfsv32::~rfsv32()
{
	bufferStore a;
	a.addStringT("Close");
	if (status == E_PSI_GEN_NONE)
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
	status = E_PSI_FILE_DISC;
	a.addStringT(getConnectName());
	if (skt->sendBufferStore(a)) {
		if (skt->getBufferStore(a) == 1) {
			if (!strcmp(a.getString(0), "Ok"))
				status = E_PSI_GEN_NONE;
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
	return "SYS$RFSV";
}

char *rfsv32::
convertSlash(const char *name)
{
	char *n = strdup(name);
	for (char *p = n; *p; p++)
		if (*p == '/')
			*p = '\\';
	return n;
}

long rfsv32::
fopen(long attr, const char *name, long &handle)
{
	bufferStore a;
	char *n = convertSlash(name);
	a.addDWord(attr);
	a.addWord(strlen(n));
	a.addString(n);
	free(n);
	if (!sendCommand(OPEN_FILE, a))
		return E_PSI_FILE_DISC;
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
		return E_PSI_FILE_DISC;
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
	char *n = convertSlash(name);
	a.addDWord(attr);
	a.addWord(strlen(n));
	a.addString(n);
	free(n);
	if (!sendCommand(CREATE_FILE, a))
		return E_PSI_FILE_DISC;
	long res = getResponse(a);
	if (!res && a.getLen() == 4)
		handle = a.getDWord(0);
	return res;
}

long rfsv32::
freplacefile(long attr, const char *name, long &handle)
{
	bufferStore a;
	char *n = convertSlash(name);
	a.addDWord(attr);
	a.addWord(strlen(n));
	a.addString(n);
	free(n);
	if (!sendCommand(REPLACE_FILE, a))
		return E_PSI_FILE_DISC;
	long res = getResponse(a);
	if (!res && a.getLen() == 4)
		handle = a.getDWord(0);
	return res;
}

long rfsv32::
fopendir(long attr, const char *name, long &handle)
{
	bufferStore a;
	char *n = convertSlash(name);
	a.addDWord(attr);
	a.addWord(strlen(n));
	a.addString(n);
	free(n);
	if (!sendCommand(OPEN_DIR, a))
		return E_PSI_FILE_DISC;
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
		return E_PSI_FILE_DISC;
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
	micro += 3600; /* 1 hour PJC */
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
	micro -= 3600; /* 1 hour PJC */
	micro *= (unsigned long long)1000000;
	microLo = (micro & (unsigned long long)0x0FFFFFFFF);
	micro >>= 32;
	microHi = (micro & (unsigned long long)0x0FFFFFFFF);
}

long rfsv32::
dir(const char *name, bufferArray * files)
{
	long handle;
	long res = fopendir(EPOC_ATTR_HIDDEN | EPOC_ATTR_SYSTEM | EPOC_ATTR_DIRECTORY, name, handle);
	if (res != 0)
		return res;

	while (1) {
		bufferStore a;
		a.addDWord(handle);
		if (!sendCommand(READ_DIR, a))
			return E_PSI_FILE_DISC;
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
	if (res == E_PSI_FILE_EOF)
		res = 0;
	fclose(handle);
	return res;
}

// beware this returns static data
char * rfsv32::
opAttr(long attr)
{
	static char buf[10];
	buf[0] = ((attr & rfsv32::EPOC_ATTR_DIRECTORY) ? 'd' : '-');
	buf[1] = ((attr & rfsv32::EPOC_ATTR_RONLY) ? '-' : 'w');
	buf[2] = ((attr & rfsv32::EPOC_ATTR_HIDDEN) ? 'h' : '-');
	buf[3] = ((attr & rfsv32::EPOC_ATTR_SYSTEM) ? 's' : '-');
	buf[4] = ((attr & rfsv32::EPOC_ATTR_ARCHIVE) ? 'a' : '-');
	buf[5] = ((attr & rfsv32::EPOC_ATTR_VOLUME) ? 'v' : '-');
	buf[6] = ((attr & rfsv32::EPOC_ATTR_NORMAL) ? 'n' : '-');
	buf[7] = ((attr & rfsv32::EPOC_ATTR_TEMPORARY) ? 't' : '-');
	buf[8] = ((attr & rfsv32::EPOC_ATTR_COMPRESSED) ? 'c' : '-');
	buf[9] = '\0';
	return (char *) (&buf);
}

long rfsv32::
opMode(long mode)
{
	long ret = 0;

	ret |= (((mode & 03) == PSI_O_RDONLY) ? 0 : EPOC_OMODE_READ_WRITE);
	if (!ret)
		ret |= (mode & PSI_O_EXCL) ? 0 : EPOC_OMODE_SHARE_READERS;
	return ret;
}

long rfsv32::
fgetmtime(const char *name, long *mtime)
{
	bufferStore a;
	char *n = convertSlash(name);
	a.addWord(strlen(n));
	a.addString(n);
	free(n);
	if (!sendCommand(MODIFIED, a))
		return E_PSI_FILE_DISC;
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
	char *n = convertSlash(name);
	time2micro(mtime, microHi, microLo);
	a.addDWord(microLo);
	a.addDWord(microHi);
	a.addWord(strlen(n));
	a.addString(n);
	free(n);
	if (!sendCommand(SET_MODIFIED, a))
		return E_PSI_FILE_DISC;
	long res = getResponse(a);
	if (res != 0)
		return res;
	return 0;
}

long rfsv32::
fgetattr(const char *name, long *attr)
{
	bufferStore a;
	char *n = convertSlash(name);
	a.addWord(strlen(n));
	a.addString(n);
	free(n);
	if (!sendCommand(ATT, a))
		return E_PSI_FILE_DISC;
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
	char *n = convertSlash(name);
	a.addWord(strlen(n));
	a.addString(n);
	free(n);
	if (!sendCommand(REMOTE_ENTRY, a))
		return E_PSI_FILE_DISC;
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
	char *n = convertSlash(name);
	a.addDWord(seta);
	a.addDWord(unseta);
	a.addWord(strlen(n));
	a.addString(n);
	free(n);
	if (!sendCommand(SET_ATT, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

long rfsv32::
dircount(const char *name, long *count)
{
	long handle;
	long res = fopendir(EPOC_ATTR_HIDDEN | EPOC_ATTR_SYSTEM | EPOC_ATTR_DIRECTORY, name, handle);
	*count = 0;
	if (res != 0)
		return res;

	while (1) {
		bufferStore a;
		a.addDWord(handle);
		if (!sendCommand(READ_DIR, a))
			return E_PSI_FILE_DISC;
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
	if (res == E_PSI_FILE_EOF)
		res = 0;
	return res;
}

long rfsv32::
devlist(long *devbits)
{
	bufferStore a;
	long res;

	if (!sendCommand(GET_DRIVE_LIST, a))
		return E_PSI_FILE_DISC;
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
	if (status == E_PSI_FILE_DISC) {
		reconnect();
		if (status == E_PSI_FILE_DISC)
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
		status = E_PSI_FILE_DISC;
	return result;
}

long rfsv32::
getResponse(bufferStore & data)
{
	if (skt->getBufferStore(data) == 1 &&
	    data.getWord(0) == 0x11) {
		long ret = data.getDWord(4);
		data.discardFirstBytes(8);
		return err2psierr(ret);
	} else
		status = E_PSI_FILE_DISC;
	return status;
}

long rfsv32::
fread(long handle, unsigned char *buf, long len)
{
	long res;
	long count = 0;

	do {
		bufferStore a;
		a.addDWord(handle);
		a.addDWord(((len-count) > RFSV_SENDLEN)?RFSV_SENDLEN:(len-count));
		if (!sendCommand(READ_FILE, a))
			return E_PSI_FILE_DISC;
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
fwrite(long handle, unsigned char *buf, long len)
{
	long res;
	long total = 0;
	long count;

	do {
		count = ((len - total) > RFSV_SENDLEN)?RFSV_SENDLEN:(len - total); 
		bufferStore a;
		bufferStore tmp((unsigned char *)buf, count);
		a.addDWord(handle);
		a.addBuff(tmp);
		if (!sendCommand(WRITE_FILE, a))
			return E_PSI_FILE_DISC;
		if ((res = getResponse(a)) != 0)
			return res;
		total += count;
		buf += count;
	} while ((total < len) && (count > 0));
	return total;
}

long rfsv32::
copyFromPsion(const char *from, const char *to, cpCallback_t cb)
{
	long handle;
	long res;
	long len;

	if ((res = fopen(EPOC_OMODE_SHARE_READERS | EPOC_OMODE_BINARY, from, handle)) != 0)
		return res;
	ofstream op(to);
	if (!op) {
		fclose(handle);
		return -1;
	}
	unsigned char *buff = new unsigned char[RFSV_SENDLEN];
	do {
		if ((len = fread(handle, buff, RFSV_SENDLEN)) > 0)
			op.write(buff, len);
		if (cb) {
			if (!cb(len)) {
				len = E_PSI_FILE_CANCEL;
				break;
			}
		}
	} while (len > 0);
	delete[]buff;
	fclose(handle);
	op.close();
	return len;
}

long rfsv32::
copyToPsion(const char *from, const char *to, cpCallback_t cb)
{
	long handle;
	long res;

	ifstream ip(from);
	if (!ip)
		return E_PSI_FILE_NXIST;
	res = fcreatefile(EPOC_OMODE_BINARY | EPOC_OMODE_SHARE_EXCLUSIVE | EPOC_OMODE_READ_WRITE, to, handle);
	if (res != 0) {
		res = freplacefile(EPOC_OMODE_BINARY | EPOC_OMODE_SHARE_EXCLUSIVE | EPOC_OMODE_READ_WRITE, to, handle);
		if (res != 0)
			return res;
	}
	unsigned char *buff = new unsigned char[RFSV_SENDLEN];
	int total = 0;
	while (ip && !ip.eof()) {
		ip.read(buff, RFSV_SENDLEN);
		bufferStore tmp(buff, ip.gcount());
		int len = tmp.getLen();
		total += len;
		if (len == 0)
			break;
		bufferStore a;
		a.addDWord(handle);
		a.addBuff(tmp);
		if (!sendCommand(WRITE_FILE, a)) {
			ip.close();
			delete[]buff;
			return E_PSI_FILE_DISC;
		}
		res = getResponse(a);
		if (res) {
			fclose(handle);
			ip.close();
			delete[]buff;
			return res;
		}
		if (cb) {
			if (!cb(len)) {
				fclose(handle);
				ip.close();
				delete[]buff;
				return E_PSI_FILE_CANCEL;
			}
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
		return E_PSI_FILE_DISC;
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
		return E_PSI_GEN_ARG;

	if ((mode == PSI_SEEK_CUR) && (pos >= 0)) {
		/* get and save current position */
		a.addDWord(0);
		a.addDWord(handle);
		a.addDWord(PSI_SEEK_CUR);
		if (!sendCommand(SEEK_FILE, a))
			return E_PSI_FILE_DISC;
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
			return E_PSI_FILE_DISC;
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
			return E_PSI_FILE_DISC;
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
		return E_PSI_FILE_DISC;
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
			return E_PSI_FILE_DISC;
		if ((res = getResponse(a)) != 0)
			return res;
		a.addDWord(calcpos);
		a.addDWord(handle);
		a.addDWord(PSI_SEEK_SET);
		if (!sendCommand(SEEK_FILE, a))
			return E_PSI_FILE_DISC;
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
	char *n = convertSlash(name);
	if (strlen(n) && (n[strlen(n) - 1] != '\\')) {
		a.addWord(strlen(n) + 1);
		a.addString(n);
		a.addByte('\\');
	} else {
		a.addWord(strlen(n));
		a.addString(n);
	}
	free(n);
	if (!sendCommand(MK_DIR_ALL, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

long rfsv32::
rmdir(const char *name)
{
	bufferStore a;
	char *n = convertSlash(name);
	if (strlen(n) && (n[strlen(n) - 1] != '\\')) {
		a.addWord(strlen(n) + 1);
		a.addString(n);
		a.addByte('\\');
	} else {
		a.addWord(strlen(n));
		a.addString(n);
	}
	free(n);
	if (!sendCommand(RM_DIR, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

long rfsv32::
rename(const char *oldname, const char *newname)
{
	bufferStore a;
	char *on = convertSlash(oldname);
	char *nn = convertSlash(newname);
	a.addWord(strlen(on));
	a.addString(on);
	a.addWord(strlen(nn));
	a.addString(nn);
	free(on);
	free(nn);
	if (!sendCommand(RENAME, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

long rfsv32::
remove(const char *name)
{
	bufferStore a;
	char *n = convertSlash(name);
	a.addWord(strlen(n));
	a.addString(n);
	free(n);
	if (!sendCommand(DELETE, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

static long e2psi[] = {
	rfsv::E_PSI_FILE_DIRFULL,	// -43
	rfsv::E_PSI_GEN_POWER,	// -42
	rfsv::E_PSI_GEN_DIVIDE,	// -41
	rfsv::E_PSI_FILE_TOOBIG,	// -40
	rfsv::E_PSI_FILE_ABORT,	// -39
	rfsv::E_PSI_GEN_DESCR,	// -38
	rfsv::E_PSI_GEN_LIB,	// -37
	rfsv::E_PSI_FILE_NDISC,	// -36
	rfsv::E_PSI_FILE_DISC,	// -35
	rfsv::E_PSI_FILE_CONNECT,	// -34
	rfsv::E_PSI_FILE_RETRAN,	// -33
	rfsv::E_PSI_FILE_PARITY,	// -32
	rfsv::E_PSI_FILE_OVERRUN,	// -31
	rfsv::E_PSI_FILE_FRAME,	// -30
	rfsv::E_PSI_FILE_LINE,	// -29
	rfsv::E_PSI_FILE_NAME,	// -28
	rfsv::E_PSI_FILE_DRIVER,	// -27
	rfsv::E_PSI_FILE_FULL,	// -26
	rfsv::E_PSI_FILE_EOF,	// -25
	rfsv::E_PSI_GEN_FSYS,	// -24
	rfsv::E_PSI_FILE_WRITE,	// -23
	rfsv::E_PSI_FILE_LOCKED,	// -22
	rfsv::E_PSI_FILE_ACCESS,	// -21
	rfsv::E_PSI_FILE_CORRUPT,	// -20
	rfsv::E_PSI_FILE_UNKNOWN,	// -19
	rfsv::E_PSI_FILE_NOTREADY,	// -18
	rfsv::E_PSI_FILE_COMPLETION,	// -17
	rfsv::E_PSI_GEN_BUSY,	// -16
	rfsv::E_PSI_GEN_TERMINATED,	// -15
	rfsv::E_PSI_GEN_INUSE,	// -14
	rfsv::E_PSI_GEN_DIED,	// -13
	rfsv::E_PSI_FILE_DIR,	// -12
	rfsv::E_PSI_FILE_EXIST,	// -11
	rfsv::E_PSI_GEN_UNDER,	// -10
	rfsv::E_PSI_GEN_OVER,	// -9
	rfsv::E_PSI_FILE_HANDLE,	// -8
	rfsv::E_PSI_GEN_RANGE,	// -7
	rfsv::E_PSI_GEN_ARG,	// -6
	rfsv::E_PSI_GEN_NSUP,	// -5
	rfsv::E_PSI_GEN_NOMEMORY,	// -4
	rfsv::E_PSI_FILE_CANCEL,	// -3
	rfsv::E_PSI_GEN_FAIL,	// -2
	rfsv::E_PSI_FILE_NXIST,	// -1
	rfsv::E_PSI_GEN_NONE	// 0
};

long rfsv32::
err2psierr(long status)
{
	if ((status > E_EPOC_NONE) || (status < E_EPOC_DIR_FULL)) {
		cerr << "FATAL: inavlid error-code" << endl;
		return -999;
	}
	return e2psi[status - E_EPOC_DIR_FULL];
}
