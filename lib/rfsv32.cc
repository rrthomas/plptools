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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stream.h>
#include <stdlib.h>
#include <fstream>
#include <iomanip>
#include <time.h>
#include <string>

#include "rfsv32.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "bufferarray.h"
#include "plpdirent.h"

rfsv32::rfsv32(ppsocket * _skt)
{
    skt = _skt;
    serNum = 0;
    status = rfsv::E_PSI_FILE_DISC;
    reset();
}

Enum<rfsv::errs> rfsv32::
fopen(u_int32_t attr, const char *name, u_int32_t &handle)
{
    bufferStore a;
    string n = convertSlash(name);
    a.addDWord(attr);
    a.addWord(n.size());
    a.addString(n.c_str());
    if (!sendCommand(OPEN_FILE, a))
	return E_PSI_FILE_DISC;
    Enum<rfsv::errs> res = getResponse(a);
    if (res == E_PSI_GEN_NONE && a.getLen() == 4) {
	handle = a.getDWord(0);
	return E_PSI_GEN_NONE;
    }
    return res;
}

Enum<rfsv::errs> rfsv32::
mktemp(u_int32_t &handle, string &tmpname)
{
    bufferStore a;
    if (!sendCommand(TEMP_FILE, a))
	return E_PSI_FILE_DISC;
    Enum<rfsv::errs> res = getResponse(a);
    if (res == E_PSI_GEN_NONE) {
	handle = a.getDWord(0);
	tmpname = a.getString(6);
    }
    return res;
}

Enum<rfsv::errs> rfsv32::
fcreatefile(u_int32_t attr, const char *name, u_int32_t &handle)
{
    bufferStore a;
    string n = convertSlash(name);
    a.addDWord(attr);
    a.addWord(n.size());
    a.addString(n.c_str());
    if (!sendCommand(CREATE_FILE, a))
	return E_PSI_FILE_DISC;
    Enum<rfsv::errs> res = getResponse(a);
    if (res == E_PSI_GEN_NONE && a.getLen() == 4)
	handle = a.getDWord(0);
    return res;
}

Enum<rfsv::errs> rfsv32::
freplacefile(const u_int32_t attr, const char * const name, u_int32_t &handle)
{
    bufferStore a;
    string n = convertSlash(name);
    a.addDWord(attr);
    a.addWord(n.size());
    a.addString(n.c_str());
    if (!sendCommand(REPLACE_FILE, a))
	return E_PSI_FILE_DISC;
    Enum<rfsv::errs> res = getResponse(a);
    if (res == E_PSI_GEN_NONE && a.getLen() == 4)
	handle = a.getDWord(0);
    return res;
}

Enum<rfsv::errs> rfsv32::
fopendir(const u_int32_t attr, const char * const name, u_int32_t &handle)
{
    bufferStore a;
    string n = convertSlash(name);
    a.addDWord(attr | EPOC_ATTR_GETUID);
    a.addWord(n.size());
    a.addString(n.c_str());
    if (!sendCommand(OPEN_DIR, a))
	return E_PSI_FILE_DISC;
    Enum<rfsv::errs> res = getResponse(a);
    if (!res && a.getLen() == 4)
	handle = a.getDWord(0);
    return res;
}

Enum<rfsv::errs> rfsv32::
fclose(u_int32_t handle)
{
    bufferStore a;
    a.addDWord(handle);
    if (!sendCommand(CLOSE_HANDLE, a))
	return E_PSI_FILE_DISC;
    return getResponse(a);
}

Enum<rfsv::errs> rfsv32::
opendir(const u_int32_t attr, const char *name, rfsvDirhandle &dH) {
    u_int32_t handle;
    Enum<rfsv::errs> res = fopendir(std2attr(attr), name, handle);
    dH.h = handle;
    dH.b.init();
    return res;
}

Enum<rfsv::errs> rfsv32::
closedir(rfsvDirhandle &dH) {
    return fclose(dH.h);
}

Enum<rfsv::errs> rfsv32::
readdir(rfsvDirhandle &dH, PlpDirent &e) {
    Enum<rfsv::errs> res = E_PSI_GEN_NONE;

    if (dH.b.getLen() < 17) {
	dH.b.init();
	dH.b.addDWord(dH.h);
	if (!sendCommand(READ_DIR, dH.b))
	    return E_PSI_FILE_DISC;
	res = getResponse(dH.b);
    }
    if ((res == E_PSI_GEN_NONE) && (dH.b.getLen() > 16)) {
	long shortLen   = dH.b.getDWord(0);
	long longLen    = dH.b.getDWord(32);

	e.attr    = attr2std(dH.b.getDWord(4));
	e.size    = dH.b.getDWord(8);
	e.UID     = PlpUID(dH.b.getDWord(20), dH.b.getDWord(24), dH.b.getDWord(28));
	e.time    = PsiTime(dH.b.getDWord(16), dH.b.getDWord(12));
	e.name    = "";
	e.attrstr = string(attr2String(e.attr));

	int d = 36;
	for (int i = 0; i < longLen; i++, d++)
	    e.name += dH.b.getByte(d);
	while (d % 4)
	    d++;
	d += shortLen;
	while (d % 4)
	    d++;
	dH.b.discardFirstBytes(d);
    }
    return res;
}

Enum<rfsv::errs> rfsv32::
dir(const char *name, PlpDir &files)
{
    rfsvDirhandle h;
    files.clear();
    Enum<rfsv::errs> res = opendir(PSI_A_HIDDEN | PSI_A_SYSTEM | PSI_A_DIR, name, h);
    while (res == E_PSI_GEN_NONE) {
	PlpDirent e;
	res = readdir(h, e);
	if (res == E_PSI_GEN_NONE)
	    files.push_back(e);
    }
    closedir(h);
    if (res == E_PSI_FILE_EOF)
	res = E_PSI_GEN_NONE;
    return res;
}

u_int32_t rfsv32::
opMode(const u_int32_t mode)
{
    u_int32_t ret = 0;

    ret |= (((mode & 03) == PSI_O_RDONLY) ? 0 : EPOC_OMODE_READ_WRITE);
    if (!ret)
	ret |= (mode & PSI_O_EXCL) ? 0 : EPOC_OMODE_SHARE_READERS;
    return ret;
}

Enum<rfsv::errs> rfsv32::
fgetmtime(const char * const name, PsiTime &mtime)
{
    bufferStore a;
    string n = convertSlash(name);
    a.addWord(n.size());
    a.addString(n.c_str());
    if (!sendCommand(MODIFIED, a))
	return E_PSI_FILE_DISC;
    Enum<rfsv::errs> res = getResponse(a);
    if (res != E_PSI_GEN_NONE)
	return res;
    mtime.setPsiTime(a.getDWord(4), a.getDWord(0));
    return res;
}

Enum<rfsv::errs> rfsv32::
fsetmtime(const char * const name, PsiTime mtime)
{
    bufferStore a;
    string n = convertSlash(name);
    a.addDWord(mtime.getPsiTimeLo());
    a.addDWord(mtime.getPsiTimeHi());
    a.addWord(n.size());
    a.addString(n.c_str());
    if (!sendCommand(SET_MODIFIED, a))
	return E_PSI_FILE_DISC;
    return getResponse(a);
}

Enum<rfsv::errs> rfsv32::
fgetattr(const char * const name, u_int32_t &attr)
{
    bufferStore a;
    string n = convertSlash(name);
    a.addWord(n.size());
    a.addString(n.c_str());
    if (!sendCommand(ATT, a))
	return E_PSI_FILE_DISC;
    Enum<rfsv::errs> res = getResponse(a);
    if (res != E_PSI_GEN_NONE)
	return res;
    attr = attr2std(a.getDWord(0));
    return res;
}

Enum<rfsv::errs> rfsv32::
fgeteattr(const char * const name, PlpDirent &e)
{
    bufferStore a;
    string n = convertSlash(name);
    a.addWord(n.size());
    a.addString(n.c_str());
    const char *p = strrchr(n.c_str(), '\\');
    if (p)
	p++;
    else
	p = n.c_str();
    e.name = p;

    if (!sendCommand(REMOTE_ENTRY, a))
	return E_PSI_FILE_DISC;
    Enum<rfsv::errs> res = getResponse(a);
    if (res != E_PSI_GEN_NONE)
	return res;
    // long shortLen = a.getDWord(0);
    // long longLen = a.getDWord(32);

    e.attr    = attr2std(a.getDWord(4));
    e.size    = a.getDWord(8);
    e.UID     = PlpUID(a.getDWord(20), a.getDWord(24), a.getDWord(28));
    e.time    = PsiTime(a.getDWord(16), a.getDWord(12));
    e.attrstr = string(attr2String(e.attr));

    return res;
}

Enum<rfsv::errs> rfsv32::
fsetattr(const char * const name, const u_int32_t seta, const u_int32_t unseta)
{
    bufferStore a;
    string n = convertSlash(name);
    a.addDWord(std2attr(seta));
    a.addDWord(std2attr(unseta));
    a.addWord(n.size());
    a.addString(n.c_str());
    if (!sendCommand(SET_ATT, a))
	return E_PSI_FILE_DISC;
    return getResponse(a);
}

Enum<rfsv::errs> rfsv32::
dircount(const char * const name, u_int32_t &count)
{
    u_int32_t handle;
    Enum<rfsv::errs> res = fopendir(EPOC_ATTR_HIDDEN | EPOC_ATTR_SYSTEM | EPOC_ATTR_DIRECTORY, name, handle);
    count = 0;
    if (res != E_PSI_GEN_NONE)
	return res;

    while (1) {
	bufferStore a;
	a.addDWord(handle);
	if (!sendCommand(READ_DIR, a))
	    return E_PSI_FILE_DISC;
	res = getResponse(a);
	if (res != E_PSI_GEN_NONE)
	    break;
	while (a.getLen() > 16) {
	    int d = 36 + a.getDWord(32);
	    while (d % 4)
		d++;
	    d += a.getDWord(0);
	    while (d % 4)
		d++;
	    a.discardFirstBytes(d);
	    count++;
	}
    }
    fclose(handle);
    if (res == E_PSI_FILE_EOF)
	res = E_PSI_GEN_NONE;
    return res;
}

Enum<rfsv::errs> rfsv32::
devlist(u_int32_t &devbits)
{
    bufferStore a;
    Enum<rfsv::errs> res;

    if (!sendCommand(GET_DRIVE_LIST, a))
	return E_PSI_FILE_DISC;
    res = getResponse(a);
    devbits = 0;
    if ((res == E_PSI_GEN_NONE) && (a.getLen() == 26)) {
	for (int i = 25; i >= 0; i--) {
	    devbits <<= 1;
	    if (a.getByte(i) != 0)
		devbits |= 1;
	}
    }
    return res;
}

Enum<rfsv::errs> rfsv32::
devinfo(const u_int32_t dev, PlpDrive &drive)
{
    bufferStore a;
    Enum<rfsv::errs> res;

    a.addDWord(dev);
    if (!sendCommand(DRIVE_INFO, a))
	return E_PSI_FILE_DISC;
    res = getResponse(a);
    if (res == E_PSI_GEN_NONE) {
	drive.setMediaType(a.getDWord(0));
	drive.setDriveAttribute(a.getDWord(8));
	drive.setMediaAttribute(a.getDWord(12));
	drive.setUID(a.getDWord(16));
	drive.setSize(a.getDWord(20), a.getDWord(24));
	drive.setSpace(a.getDWord(28), a.getDWord(32));
	a.addByte(0);
	drive.setName('A' + dev, a.getString(40));
    }
    return res;
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
    if (!result) {
	reconnect();
	result = skt->sendBufferStore(a);
	if (!result)
	    status = E_PSI_FILE_DISC;
    }
    return result;
}

Enum<rfsv::errs> rfsv32::
getResponse(bufferStore & data)
{
    if (skt->getBufferStore(data) == 1 &&
	data.getWord(0) == 0x11) {
	int32_t ret = data.getDWord(4);
	data.discardFirstBytes(8);
	return err2psierr(ret);
    } else
	status = E_PSI_FILE_DISC;
    return status;
}

Enum<rfsv::errs> rfsv32::
fread(const u_int32_t handle, unsigned char * const buf, const u_int32_t len, u_int32_t &count)
{
    Enum<rfsv::errs> res;
    count = 0;
    long l;
    unsigned char *p = buf;

    do {
	bufferStore a;
	a.addDWord(handle);
	a.addDWord(((len - count) > RFSV_SENDLEN)?RFSV_SENDLEN:(len - count));
	if (!sendCommand(READ_FILE, a))
	    return E_PSI_FILE_DISC;
	if ((res = getResponse(a)) != E_PSI_GEN_NONE)
	    return res;
	if ((l = a.getLen()) > 0) {
	    memcpy(p, a.getString(), l);
	    count += l;
	    p += l;
	}
    } while ((count < len) && (l > 0));
    return res;
}

Enum<rfsv::errs> rfsv32::
fwrite(const u_int32_t handle, const unsigned char * const buf, const u_int32_t len, u_int32_t &count)
{
    Enum<rfsv::errs> res;
    const unsigned char *p = buf;
    long l;

    count = 0;
    do {
	l = ((len - count) > RFSV_SENDLEN)?RFSV_SENDLEN:(len - count);
	if (l > 0) {
	    bufferStore a;
	    bufferStore tmp(p, l);
	    a.addDWord(handle);
	    a.addBuff(tmp);
	    if (!sendCommand(WRITE_FILE, a))
		return E_PSI_FILE_DISC;
	    if ((res = getResponse(a)) != E_PSI_GEN_NONE)
		return res;
	    count += l;
	    p += l;
	}
    } while ((count < len) && (l > 0));
    return res;
}

Enum<rfsv::errs> rfsv32::
copyFromPsion(const char *from, const char *to, void *ptr, cpCallback_t cb)
{
    Enum<rfsv::errs> res;
    u_int32_t handle;
    u_int32_t len;
    u_int32_t total = 0;

    if ((res = fopen(EPOC_OMODE_SHARE_READERS | EPOC_OMODE_BINARY, from, handle)) != E_PSI_GEN_NONE)
	return res;
    ofstream op(to);
    if (!op) {
	fclose(handle);
	return E_PSI_GEN_FAIL;
    }
    unsigned char *buff = new unsigned char[RFSV_SENDLEN];
    do {
	if ((res = fread(handle, buff, RFSV_SENDLEN, len)) == E_PSI_GEN_NONE) {
	    op.write(buff, len);
	    total += len;
	    if (cb && !cb(ptr, total))
		res = E_PSI_FILE_CANCEL;
	}
    } while ((len > 0) && (res == E_PSI_GEN_NONE));
    delete[]buff;
    fclose(handle);
    op.close();
    return res;
}

Enum<rfsv::errs> rfsv32::
copyToPsion(const char *from, const char *to, void *ptr, cpCallback_t cb)
{
    u_int32_t handle;
    Enum<rfsv::errs> res;

    ifstream ip(from);
    if (!ip)
	return E_PSI_FILE_NXIST;
    res = fcreatefile(EPOC_OMODE_BINARY | EPOC_OMODE_SHARE_EXCLUSIVE | EPOC_OMODE_READ_WRITE, to, handle);
    if (res != E_PSI_GEN_NONE) {
	res = freplacefile(EPOC_OMODE_BINARY | EPOC_OMODE_SHARE_EXCLUSIVE | EPOC_OMODE_READ_WRITE, to, handle);
	if (res != E_PSI_GEN_NONE)
	    return res;
    }
    unsigned char *buff = new unsigned char[RFSV_SENDLEN];
    u_int32_t total = 0;
    while (ip && !ip.eof() && (res == E_PSI_GEN_NONE)) {
	u_int32_t len;
	ip.read(buff, RFSV_SENDLEN);
	if ((res = fwrite(handle, buff, ip.gcount(), len)) == E_PSI_GEN_NONE) {
	    total += len;
	    if (cb && !cb(ptr, total))
		res = E_PSI_FILE_CANCEL;
	}
    }
    fclose(handle);
    ip.close();
    delete[]buff;
    return res;
}

Enum<rfsv::errs> rfsv32::
copyOnPsion(const char *from, const char *to, void *ptr, cpCallback_t cb)
{
    u_int32_t handle_from;
    u_int32_t handle_to;
    PlpDirent from_e;
    Enum<rfsv::errs> res;

    if ((res = fgeteattr(from, from_e)) != E_PSI_GEN_NONE)
	return res;
    if ((res = fopen(EPOC_OMODE_SHARE_READERS | EPOC_OMODE_BINARY, from, handle_from))
	!= E_PSI_GEN_NONE)
	return res;
    res = fcreatefile(EPOC_OMODE_BINARY | EPOC_OMODE_SHARE_EXCLUSIVE | EPOC_OMODE_READ_WRITE, to, handle_to);
    if (res != E_PSI_GEN_NONE) {
	res = freplacefile(EPOC_OMODE_BINARY | EPOC_OMODE_SHARE_EXCLUSIVE | EPOC_OMODE_READ_WRITE, to, handle_to);
	if (res != E_PSI_GEN_NONE) {
	    fclose(handle_from);
	    return res;
	}
    }

    u_int32_t total = 0;
    while (res == E_PSI_GEN_NONE) {
	bufferStore b;
	b.addDWord(RFSV_SENDLEN * 10);
	b.addDWord(handle_to);
	b.addDWord(handle_from);
	if (!sendCommand(READ_WRITE_FILE, b))
	    return E_PSI_FILE_DISC;
	res = getResponse(b);
	if (res != E_PSI_GEN_NONE)
	    break;
	if (b.getLen() != 4) {
	    res = E_PSI_GEN_FAIL;
	    break;
	}
	u_int32_t len = b.getDWord(0);
	total += len;
	if (cb && !cb(ptr, total))
	    res = E_PSI_FILE_CANCEL;
	if (len != (RFSV_SENDLEN * 10))
	    break;
    }
    fclose(handle_from);
    fclose(handle_to);
    if (res != E_PSI_GEN_NONE)
	remove(to);
    return res;
}

Enum<rfsv::errs> rfsv32::
fsetsize(u_int32_t handle, u_int32_t size)
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
Enum<rfsv::errs> rfsv32::
fseek(const u_int32_t handle, const int32_t pos, const u_int32_t mode, u_int32_t &resultpos)
{
    bufferStore a;
    Enum<rfsv::errs> res;
    u_int32_t savpos = 0;
    u_int32_t calcpos = 0;
    int32_t mypos = pos;
    u_int32_t realpos;

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

    if ((mode == PSI_SEEK_CUR) && (mypos >= 0)) {
	/* get and save current position */
	a.addDWord(0);
	a.addDWord(handle);
	a.addDWord(PSI_SEEK_CUR);
	if (!sendCommand(SEEK_FILE, a))
	    return E_PSI_FILE_DISC;
	if ((res = getResponse(a)) != E_PSI_GEN_NONE)
	    return res;
	savpos = a.getDWord(0);
	if (mypos == 0) {
	    resultpos = savpos;
	    return res;
	}
	a.init();
    }
    if ((mode == PSI_SEEK_END) && (mypos >= 0)) {
	/* get and save end position */
	a.addDWord(0);
	a.addDWord(handle);
	a.addDWord(PSI_SEEK_END);
	if (!sendCommand(SEEK_FILE, a))
	    return E_PSI_FILE_DISC;
	if ((res = getResponse(a)) != E_PSI_GEN_NONE)
	    return res;
	savpos = a.getDWord(0);
	if (mypos == 0) {
	    resultpos = savpos;
	    return res;
	}
	/* Expand file */
	a.init();
	a.addDWord(handle);
	a.addDWord(savpos + mypos);
	if (!sendCommand(SET_SIZE, a))
	    return E_PSI_FILE_DISC;
	if ((res = getResponse(a)) != E_PSI_GEN_NONE)
	    return res;
	mypos = 0;
	a.init();
    }
    /* Now the real seek */
    a.addDWord(mypos);
    a.addDWord(handle);
    a.addDWord(mode);
    if (!sendCommand(SEEK_FILE, a))
	return E_PSI_FILE_DISC;
    if ((res = getResponse(a)) != E_PSI_GEN_NONE)
	return res;
    realpos = a.getDWord(0);
    switch (mode) {
	case PSI_SEEK_SET:
	    calcpos = mypos;
	    break;
	case PSI_SEEK_CUR:
	    calcpos = savpos + mypos;
	    break;
	case PSI_SEEK_END:
	    resultpos = realpos;
	    return res;
	    break;
    }
    if (calcpos > realpos) {
	/* Beyond end of file */
	a.init();
	a.addDWord(handle);
	a.addDWord(calcpos);
	if (!sendCommand(SET_SIZE, a))
	    return E_PSI_FILE_DISC;
	if ((res = getResponse(a)) != E_PSI_GEN_NONE)
	    return res;
	a.addDWord(calcpos);
	a.addDWord(handle);
	a.addDWord(PSI_SEEK_SET);
	if (!sendCommand(SEEK_FILE, a))
	    return E_PSI_FILE_DISC;
	if ((res = getResponse(a)) != E_PSI_GEN_NONE)
	    return res;
	realpos = a.getDWord(0);
    }
    resultpos = realpos;
    return res;
}

Enum<rfsv::errs> rfsv32::
mkdir(const char *name)
{
    bufferStore a;
    string n = convertSlash(name);
    if (n.find_last_of('\\') != (n.size() - 1))
	n += '\\';
    a.addWord(n.size());
    a.addString(n.c_str());
    if (!sendCommand(MK_DIR_ALL, a))
	return E_PSI_FILE_DISC;
    return getResponse(a);
}

Enum<rfsv::errs> rfsv32::
rmdir(const char *name)
{
    bufferStore a;
    string n = convertSlash(name);
    if (n.find_last_of('\\') != (n.size() - 1))
	n += '\\';
    a.addWord(n.size());
    a.addString(n.c_str());
    if (!sendCommand(RM_DIR, a))
	return E_PSI_FILE_DISC;
    return getResponse(a);
}

Enum<rfsv::errs> rfsv32::
rename(const char *oldname, const char *newname)
{
    bufferStore a;
    string on = convertSlash(oldname);
    string nn = convertSlash(newname);
    a.addWord(on.size());
    a.addString(on.c_str());
    a.addWord(nn.size());
    a.addString(nn.c_str());
    if (!sendCommand(RENAME, a))
	return E_PSI_FILE_DISC;
    return getResponse(a);
}

Enum<rfsv::errs> rfsv32::
remove(const char *name)
{
    bufferStore a;
    string n = convertSlash(name);
    a.addWord(n.size());
    a.addString(n.c_str());
    if (!sendCommand(DELETE, a))
	return E_PSI_FILE_DISC;
    return getResponse(a);
}

Enum<rfsv::errs> rfsv32::
setVolumeName(const char drive , const char * const name)
{
    bufferStore a;
    a.addDWord(drive - 'A');
    a.addWord(strlen(name));
    a.addStringT(name);
    if (!sendCommand(SET_VOLUME_LABEL, a))
	return E_PSI_FILE_DISC;
    return getResponse(a);
}

static enum rfsv::errs e2psi[] = {
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

Enum<rfsv::errs> rfsv32::
err2psierr(int32_t status)
{
    if ((status > E_EPOC_NONE) || (status < E_EPOC_DIR_FULL)) {
	cerr << "FATAL: inavlid error-code" << endl;
	cerr << "status: " << status << " " << hex << status << endl;
	return E_PSI_INTERNAL;
    }
    return e2psi[status - E_EPOC_DIR_FULL];
}


/*
 * Translate EPOC attributes to standard attributes.
 */
u_int32_t rfsv32::
attr2std(const u_int32_t attr)
{
    long res = 0;

    // Common attributes
    if (attr & EPOC_ATTR_RONLY)
	res |= PSI_A_RDONLY;
    if (attr & EPOC_ATTR_HIDDEN)
	res |= PSI_A_HIDDEN;
    if (attr & EPOC_ATTR_SYSTEM)
	res |= PSI_A_SYSTEM;
    if (attr & EPOC_ATTR_DIRECTORY)
	res |= PSI_A_DIR;
    if (attr & EPOC_ATTR_ARCHIVE)
	res |= PSI_A_ARCHIVE;
    if (attr & EPOC_ATTR_VOLUME)
	res |= PSI_A_VOLUME;

    // EPOC-specific
    if (attr & EPOC_ATTR_NORMAL)
	res |= PSI_A_NORMAL;
    if (attr & EPOC_ATTR_TEMPORARY)
	res |= PSI_A_TEMP;
    if (attr & EPOC_ATTR_COMPRESSED)
	res |= PSI_A_COMPRESSED;

    // Do what we can for SIBO
    res |= PSI_A_READ;

    return res;
}

/*
 * Translate standard attributes to EPOC attributes.
 */
u_int32_t rfsv32::
std2attr(const u_int32_t attr)
{
    long res = 0;
    // Common attributes
    if (attr & PSI_A_RDONLY)
	res |= EPOC_ATTR_RONLY;
    if (attr & PSI_A_HIDDEN)
	res |= EPOC_ATTR_HIDDEN;
    if (attr & PSI_A_SYSTEM)
	res |= EPOC_ATTR_SYSTEM;
    if (attr & PSI_A_DIR)
	res |= EPOC_ATTR_DIRECTORY;
    if (attr & PSI_A_ARCHIVE)
	res |= EPOC_ATTR_ARCHIVE;
    if (attr & PSI_A_VOLUME)
	res |= EPOC_ATTR_VOLUME;

    // EPOC-specific
    if (attr & PSI_A_NORMAL)
	res |= EPOC_ATTR_NORMAL;
    if (attr & PSI_A_TEMP)
	res |= EPOC_ATTR_TEMPORARY;
    if (attr & PSI_A_COMPRESSED)
	res |= EPOC_ATTR_COMPRESSED;

    return res;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
