//
//  RFSV16 - An implementation of the PSION SIBO RFSV Client protocol
//
//  Copyright (C) 1999  Philip Proudman
//  Modifications for plptools:
//    Copyright (C) 1999 Matt J. Gumbley <matt@gumbley.demon.co.uk>
//    Sources: rfsv32.cc by Fritz Elfert, and rfsv16.cc by Philip Proudman
//    Descriptions of the RFSV16 protocol by Michael Pieper, Olaf Flebbe & Me.
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
#include "rfsv16.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "bufferarray.h"

rfsv16::rfsv16(ppsocket *_skt) : serNum(0)
{
	skt = _skt;
	reset();
}

// move to base class?
rfsv16::~rfsv16() 
{
	bufferStore a;
	a.addStringT("Close");
	if (status == E_PSI_GEN_NONE)
		skt->sendBufferStore(a);
	skt->closeSocket();
}

// move to base class?
void rfsv16::
reconnect()
{
cerr << "rfsv16::reconnect" << endl;
	skt->closeSocket();
	skt->reconnect();
	serNum = 0;
	reset();
}

// move to base class?
void rfsv16::
reset()
{
cerr << "rfsv16::reset" << endl;
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

// move to base class?
long rfsv16::
getStatus()
{
	return status;
}

// move to base class?
const char *rfsv16::
getConnectName()
{
	return "SYS$RFSV.*";
}

int rfsv16::
convertName(const char* orig, char *retVal)
{
	int len = strlen(orig);
	char *temp = new char [len+1];

	// FIXME: need to return 1 if OOM?
	for (int i=0; i <= len; i++) {
		if (orig[i] == '/')
			temp[i] = '\\';
		else
			temp[i] = orig[i];
	}

	if (len == 0 || temp[1] != ':') {
		// We can automatically supply a drive letter
		strcpy(retVal, DDRIVE);

		if (len == 0 || temp[0] != '\\') {
			strcat(retVal, DBASEDIR);
		}
		else {
			retVal[strlen(retVal)-1] = 0;
		}

		strcat(retVal, temp);
	}
	else {
		strcpy(retVal, temp);
	}

	delete [] temp;
	cout << retVal << endl;
	return 0;
}

long rfsv16::
fopen(long attr, const char *name, long &handle)
{ 
	bufferStore a;
	char realName[200];
	int rv = convertName(name, realName);
	if (rv) return (long)rv;

	// FIXME: anything that calls fopen should NOT do the name
	// conversion - it's just done here. 

	a.addWord(attr & 0xFFFF);
	a.addString(realName);
	a.addByte(0x00); // Needs to be manually Null-Terminated.
	if (!sendCommand(FOPEN, a))
		return E_PSI_FILE_DISC;
  
	long res = getResponse(a);
	// cerr << "fopen, getword 0 is " << hex << setw(2) << a.getWord(0) << endl;
	// cerr << "fopen, getlen is " << hex << setw(2) << a.getLen() << endl;
	// cerr << "fopen, res is " << hex << setw(2) << res << endl;
	if (!res && a.getLen() == 4 && a.getWord(0) == 0) {
		handle = (long)a.getWord(2);
		return 0;
	}
	// cerr << "fopen: Unknown response (" << attr << "," << name << ") " << a <<endl;
	return res;
}

// internal
long rfsv16::
mktemp(long *handle, char *tmpname)
{
cerr << "rfsv16::mktemp ***" << endl;
	return 0;
}

// internal and external
long rfsv16::
fcreatefile(long attr, const char *name, long &handle)
{
cerr << "rfsv16::fcreatefile ***" << endl;
	return 0;
}

// this is internal - not used by plpnfsd, unlike fcreatefile
long rfsv16::
freplacefile(long attr, const char *name, long &handle)
{
cerr << "rfsv16::freplacefile ***" << endl;
	return 0;
}

// internal
long rfsv16::
fopendir(long attr, const char *name, long &handle)
{
cerr << "rfsv16::fopendir ***" << endl;
	return 0;
}

long rfsv16::
fclose(long fileHandle)
{
	bufferStore a;
	a.addWord(fileHandle & 0xFFFF);
	if (!sendCommand(FCLOSE, a))
		return E_PSI_FILE_DISC;
	long res = getResponse(a);
	if (!res && a.getLen() == 2)
		return (long)a.getWord(0);
	cerr << "fclose: Unknown response "<< a <<endl;
	return 1;
}

long rfsv16::
dir(const char *dirName, bufferArray * files)
{
	long fileHandle;
	long res;

	long status = fopen(P_FDIR, dirName, fileHandle);
	if (status != 0) {
		return status;
	}

	while (1) {
		bufferStore a;
		a.addWord(fileHandle & 0xFFFF);
		if (!sendCommand(FDIRREAD, a))
			return E_PSI_FILE_DISC;
		res = getResponse(a);
		if (res)
			break;
		a.discardFirstBytes(4); // Don't know what these mean!
		while (a.getLen() > 16) {
			int version = a.getWord(0);
			if (version != 2) {
				cerr << "dir: not version 2" << endl;
				return 1;
			}
			int status = a.getWord(2);
			long size = a.getDWord(4);
			long date = a.getDWord(8);
			const char *s = a.getString(16);
			a.discardFirstBytes(17+strlen(s));

			bufferStore temp;
			temp.addDWord(date);
			temp.addDWord(size);
			temp.addDWord((long)status);
			temp.addStringT(s);
			files->pushBuffer(temp);
		}
	}
	if ((short int)res == E_PSI_FILE_EOF)
		res = 0;
	fclose(fileHandle);
	return res;
}

char * rfsv16::
opAttr(long attr)
{
	static char buf[11];
	buf[0] = ((attr & rfsv16::P_FAWRITE) ? 'w' : '-');
	buf[1] = ((attr & rfsv16::P_FAHIDDEN) ? 'h' : '-');
	buf[2] = ((attr & rfsv16::P_FASYSTEM) ? 's' : '-');
	buf[3] = ((attr & rfsv16::P_FAVOLUME) ? 'v' : '-');
	buf[4] = ((attr & rfsv16::P_FADIR) ? 'd' : '-');
	buf[5] = ((attr & rfsv16::P_FAMOD) ? 'm' : '-');
	buf[6] = ((attr & rfsv16::P_FAREAD) ? 'r' : '-');
	buf[7] = ((attr & rfsv16::P_FAEXEC) ? 'x' : '-');
	buf[8] = ((attr & rfsv16::P_FASTREAM) ? 'b' : '-');
	buf[9] = ((attr & rfsv16::P_FATEXT) ? 't' : '-');
        buf[10] = '\0';
	return (char *) (&buf);
}


long rfsv16::
opMode(long mode)
{
	long ret = 0;

	ret |= ((mode & 03) == PSI_O_RDONLY) ? 0 : P_FUPDATE;
	ret |= (mode & PSI_O_TRUNC) ? P_FREPLACE : 0;
	ret |= (mode & PSI_O_CREAT) ? P_FCREATE : 0;
	ret |= (mode & PSI_O_APPEND) ? P_FAPPEND : 0;
	if ((mode & 03) == PSI_O_RDONLY)
		ret |= (mode & PSI_O_EXCL) ? 0 : P_FSHARE;
	return ret;
}

long rfsv16::
fgetmtime(const char *name, long *mtime)
{
cerr << "rfsv16::fgetmtime" << endl;
	// NB: fgetattr, fgeteattr is almost identical...
	bufferStore a;
	char realName[200];
	int rv = convertName(name, realName);
	if (rv) return rv;
	a.addString(realName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	if (!sendCommand(FINFO, a))
		return E_PSI_FILE_DISC;
  
	long res = getResponse(a);
	if (res != 0)
		return res;
	if (a.getLen() == 2) {
		cerr << "fgetmtime: Error " << a.getWord(0) << " on file " << name << endl;
		return 1;
	}
	else if (a.getLen() == 18 && a.getWord(0) == 0) {
		*mtime = a.getDWord(10);
		return a.getWord(0);
	}
	cerr << "fgetmtime: Unknown response (" << name << ") " << a <<endl;
	return 1;
}

long rfsv16::
fsetmtime(const char *name, long mtime)
{
cerr << "rfsv16::fsetmtime ***" << endl;
	// I don't think there's a protocol frame that allows us to set the
	// modification time. SFDATE allows setting of creation time...
	return E_PSI_NOT_SIBO;
}

long rfsv16::
fgetattr(const char *name, long *attr)
{
	// NB: fgetmtime, fgeteattr are almost identical...
	bufferStore a;
	char realName[200];
	int rv = convertName(name, realName);
	if (rv) return rv;
	a.addString(realName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	if (!sendCommand(FINFO, a))
		return E_PSI_FILE_DISC;
  
	long res = getResponse(a);
	if (res != 0)
		return res;
	if (a.getLen() == 2) {
		cerr << "fgetattr: Error " << a.getWord(0) << " on file " << name << endl;
		return 1;
	}
	else if (a.getLen() == 18 && a.getWord(0) == 0) {
		*attr = (long)(a.getWord(4));
		return a.getWord(0);
	}
	cerr << "fgetattr: Unknown response (" << name << ") " << a <<endl;
	return 1;
}

long rfsv16::
fgeteattr(const char *name, long *attr, long *size, long *time)
{
	bufferStore a;
	char realName[200];
	int rv = convertName(name, realName);
	if (rv) return rv;
	a.addString(realName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	if (!sendCommand(FINFO, a))
		return E_PSI_FILE_DISC;
  
	long res = getResponse(a);
	if (res != 0)
		return res;
	if (a.getLen() == 2) {
		cerr << "fgeteattr: Error " << a.getWord(0) << " on file " << name << endl;
		return 1;
	}
	else if (a.getLen() == 18 && a.getWord(0) == 0) {
		*attr = (long)(a.getWord(4));
		*size = a.getDWord(6);
		*time = a.getDWord(10);
		return a.getWord(0);
	}
	cerr << "fgeteattr: Unknown response (" << name << ") " << a <<endl;
	return 1;
}

long rfsv16::
fsetattr(const char *name, long seta, long unseta)
{
cerr << "rfsv16::fsetattr" << endl;
	// seta are attributes to set; unseta are attributes to unset. Need to
	// turn this into attributes to change state and a bit mask.
	// 210000
	// 008421
	// a  shr
	long statusword = seta & (~ unseta);
	statusword ^= 0x0000001; // r bit is inverted
	long bitmask = seta | unseta;
	// cerr << "seta is   " << hex << setw(2) << setfill('0') << seta << endl;
	// cerr << "unseta is " << hex << setw(2) << setfill('0') << unseta << endl;
	// cerr << "statusword is  " << hex << setw(2) << setfill('0') << statusword << endl;
	// cerr << "bitmask is     " << hex << setw(2) << setfill('0') << bitmask << endl;
	bufferStore a;
	a.addWord(statusword & 0xFFFF);
	a.addWord(bitmask & 0xFFFF);
	a.addString(name);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	if (!sendCommand(SFSTAT, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

long rfsv16::
dircount(const char *name, long *count)
{
	long fileHandle;
	long res;
	*count = 0;

	long status = fopen(P_FDIR, name, fileHandle);
	if (status != 0) {
		return status;
	}

	while (1) {
		bufferStore a;
		a.addWord(fileHandle & 0xFFFF);
		if (!sendCommand(FDIRREAD, a))
			return E_PSI_FILE_DISC;
		res = getResponse(a);
		if (res)
			break;
		a.discardFirstBytes(4); // Don't know what these mean!
		while (a.getLen() > 16) {
			int version = a.getWord(0);
			if (version != 2) {
				cerr << "dir: not version 2" << endl;
				return 1;
			}
			// int status = a.getWord(2);
			// long size = a.getDWord(4);
			// long date = a.getDWord(8);
			const char *s = a.getString(16);
			a.discardFirstBytes(17+strlen(s));
			(*count)++;
		}
	}
	if ((short int)res == E_PSI_FILE_EOF)
		res = 0;
	fclose(fileHandle);
	return res;
}

long rfsv16::
devlist(long *devbits)
{
	long res;
	long fileHandle;
	*devbits = 0;

	// The following is taken from a trace between a Series 3c and PsiWin.
	// Hope it works! We PARSE to find the correct node, then FOPEN
	// (P_FDEVICE) this, FDEVICEREAD each entry, setting the appropriate
	// drive-letter-bit in devbits, then FCLOSE.

	bufferStore a;
	a.init();
	a.addByte(0x00); // no Name 1
	a.addByte(0x00); // no Name 2
	a.addByte(0x00); // no Name 3
	if (!sendCommand(PARSE, a))
		return E_PSI_FILE_DISC;
	res = getResponse(a);
	if (res)
		return res;

	// Find the drive to FOPEN
	char name[4] = { 'x', ':', '\\', '\0' } ;
	a.discardFirstBytes(8); // Result, fsys, dev, path, file, file, ending, flag
	/* This leaves R E M : : M : \ */
	name[0] = (char) a.getByte(5); // the M
	long status = fopen(P_FDEVICE, name, fileHandle);
	if (status != 0) {
		return status;
	}
	while (1) {
		bufferStore a;
		a.init();
		a.addWord(fileHandle & 0xFFFF);
		if (!sendCommand(FDEVICEREAD, a))
			return E_PSI_FILE_DISC;
		res = getResponse(a);
		if (res)
			break;
		a.discardFirstBytes(2); // Result
		int version = a.getWord(0);
		if (version != 2) {
			cerr << "devlist: not version 2" << endl;
			return 1; // FIXME
		}
		char drive = a.getByte(64);
		if (drive >= 'A' && drive <= 'Z') {
			int shift = (drive - 'A');
			(*devbits) |= (long) ( 1 << shift );
		}
		else {
			cerr << "devlist: non-alphabetic drive letter ("
				 << drive << ")" << endl;
		}
	}
	if ((short int)res == E_PSI_FILE_EOF)
		res = 0;
	fclose(fileHandle);
	return res;
}

char *rfsv16::
devinfo(int devnum, long *vfree, long *vtotal, long *vattr,
	long *vuniqueid)
{
	bufferStore a;
	long res;
	long fileHandle;

	// Again, this is taken from an excahnge between PsiWin and a 3c.
	// For each drive, we PARSE with its drive letter to get a response
	// (which we ignore), then do a STATUSDEVICE to get the info.

	a.init();
	a.addByte((char) (devnum + 'A')); // Name 1
	a.addByte(':');
	a.addByte(0x00);
	a.addByte(0x00); // No name 2
	a.addByte(0x00); // No name 3
	if (!sendCommand(PARSE, a))
		return NULL;
	res = getResponse(a);
	if (res) {
		// cerr << "devinfo PARSE res is " << dec << (signed short int) res << endl;
		return NULL;
	}

	a.init();
	a.addByte((char) (devnum + 'A')); // Name 1
	a.addByte(':');
	a.addByte('\\');
	a.addByte(0x00);
	if (!sendCommand(STATUSDEVICE, a))
		return NULL;
	res = getResponse(a);
	if (res) {
		// cerr << "devinfo STATUSDEVICE res is " << dec << (signed short int) res << endl;
		return NULL;
	}
	a.discardFirstBytes(2); // Result
	int type = a.getWord(2);
	int changeable = a.getWord(4);
	long size = a.getDWord(6);
	long free = a.getDWord(10);
	const char *volume = a.getString(14);
	int battery = a.getWord(30);
	const char *devicename = a.getString(62);
	*vfree = free;
	*vtotal = size;
	*vattr = type;
	*vuniqueid = 0;
	static char name[2] = { 'x', '\0' };
	name[0] = (char) (devnum + 'A');
	return strdup(name);
}

bool rfsv16::
sendCommand(enum commands cc, bufferStore & data)
{
	bool result;
	bufferStore a;
	a.addWord(cc);
	a.addWord(data.getLen());
	a.addBuff(data);
	result = skt->sendBufferStore(a);
	if (!result)
		status = E_PSI_FILE_DISC;
	return result;
}


long rfsv16::
getResponse(bufferStore & data)
{
	// getWord(2) is the size field
	// which is the body of the response not counting the command (002a) and
	// the size word.
	if (skt->getBufferStore(data) == 1 &&
	    data.getWord(0) == 0x2a &&
	    data.getWord(2) == data.getLen()-4) {
		data.discardFirstBytes(4);
		long ret = data.getWord(0);
		return ret;
	} else
		status = E_PSI_FILE_DISC;
	cerr << "rfsv16::getResponse: duff response. Size field:" <<
data.getWord(2) << " Frame size:" << data.getLen()-4 << " Result field:" <<
data.getWord(4) << endl;
	return status;
}

char * rfsv16::
opErr(long status)
{
cerr << "rfsv16::opErr 0x" << hex << setfill('0') << setw(4)
	<< status << " (" << dec << (signed short int)status << ")" << endl;
	return rfsv::opErr(status);
}

long rfsv16::
fread(long handle, unsigned char *buf, long len)
{
cerr << "rfsv16::fread ***" << endl;
	bufferStore a;
	long remaining = len;
	// Read in blocks of 291 bytes; the maximum payload for an RFSV frame.
	// ( As seen in traces ) - this isn't optimal: RFSV can handle
	// fragmentation of frames, where only the first FREAD RESPONSE frame
	// has a RESPONSE (00 2A), SIZE and RESULT field. Every subsequent frame
	// just has data, 297 bytes (or less) of it.
	const int maxblock = 291;
	long readsofar = 0;
	while (remaining) {
		long thisblock = (remaining > maxblock) ? maxblock : remaining;
cerr << "fread: " << dec << remaining << " bytes remain. This block is " << thisblock
<< " bytes." << endl;
		a.init();
		a.addWord(handle);
		a.addWord(thisblock);
		sendCommand(FREAD, a);
		long res = getResponse(a);
		remaining -= a.getLen();
// copy the data to buf

cerr << "fread getResponse returned " << dec<< (signed short int) res << " data: " << a << dec <<endl;
		if (res) {
			return res;
		}
	}
	return len;
}

long rfsv16::
fwrite(long handle, unsigned char *buf, long len)
{
cerr << "rfsv16::fwrite ***" << endl;
	return 0;
}

long rfsv16::
copyFromPsion(const char *from, const char *to, cpCallback_t cb)
{
cerr << "rfsv16::copyFromPsion" << endl;
	long handle;
	long res;
	long len;

	if ((res = fopen(P_FSHARE | P_FSTREAM, from, handle)) != 0)
		return res;
cerr << "fopen response is " << dec << (signed short int)res << endl;
	ofstream op(to);
	if (!op) {
		fclose(handle);
		return -1;
	}
	do {
		unsigned char buf[2000];
		if ((len = fread(handle, buf, sizeof(buf))) > 0)
			op.write(buf, len);
		if (cb) {
			if (!cb(len)) {
				len = E_PSI_FILE_CANCEL;
				break;
			}
		}
	} while (len > 0);

	fclose(handle);
	op.close();
	return len;
}

long rfsv16::
copyToPsion(const char *from, const char *to, cpCallback_t cb)
{
cerr << "rfsv16::copyToPsion" << endl;
	long handle;
	long res;

	ifstream ip(from);
	if (!ip)
		return E_PSI_FILE_NXIST;
	res = fcreatefile(P_FSTREAM | P_FUPDATE, to, handle);
	if (res != 0) {
		res = freplacefile(P_FSTREAM | P_FUPDATE, to, handle);
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
		if (!sendCommand(FWRITE, a)) { // FIXME: need to check params
			fclose(handle);
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

long rfsv16::
fsetsize(long handle, long size)
{
cerr << "rfsv16::fsetsize ***" << endl;
	return 0;
}

/*
 * Unix-like implementation off fseek with one
 * exception: If seeking beyond eof, the gap
 * contains garbage instead of zeroes.
 */
long rfsv16::
fseek(long handle, long pos, long mode)
{
cerr << "rfsv16::fseek ***" << endl;
	return 0;
}

long rfsv16::
mkdir(const char* dirName)
{
	char realName[200];
	int rv = convertName(dirName, realName);
	if (rv) return rv;
	bufferStore a;
	a.addString(realName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	sendCommand(MKDIR, a);
	long res = getResponse(a);
	if (!res && a.getLen() == 2) {
		// Correct response
		return a.getWord(0);
	}
	cerr << "Unknown response from mkdir "<< a <<endl;
	return 1;
}

long rfsv16::
rmdir(const char *dirName)
{
	// There doesn't seem to be an RMDIR command, but DELETE works. We
	// should probably check to see if the file is a directory first!
	return remove(dirName);
}

long rfsv16::
rename(const char *oldName, const char *newName)
{
cerr << "rfsv16::rename ***" << endl;
	char realOldName[200];
	int rv = convertName(oldName, realOldName);
	if (rv) return rv;
	char realNewName[200];
	rv = convertName(newName, realNewName);
	if (rv) return rv;
	bufferStore a;
	a.addString(realOldName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	a.addString(realNewName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	sendCommand(RENAME, a);
	long res = getResponse(a);
	if (!res && a.getLen() == 2) {
		// Correct response
		return a.getWord(0);
	}
	cerr << "Unknown response from rename "<< a <<endl;
	return 1;
}

long rfsv16::
remove(const char* psionName)
{
	char realName[200];
	int rv = convertName(psionName, realName);
	if (rv) return rv;
	bufferStore a;
	a.addString(realName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	sendCommand(DELETE, a);
	long res = getResponse(a);
	if (!res && a.getLen() == 2) {
		// Correct response
		return a.getWord(0);
	}
	cerr << "Unknown response from delete "<< a <<endl;
	return 1;
}


