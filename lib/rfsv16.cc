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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stream.h>
#include <stdlib.h>
#include <fstream.h>
#include <iomanip.h>
#include <time.h>
#include <string>

#include "bool.h"
#include "rfsv16.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "bufferarray.h"

#define	RFSV16_MAXDATALEN	852	// 640

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
	skt->closeSocket();
	skt->reconnect();
	serNum = 0;
	reset();
}

// move to base class?
void rfsv16::
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

// move to base class?
Enum<rfsv::errs> rfsv16::
getStatus()
{
	return status;
}

// move to base class?
const char *rfsv16::
getConnectName()
{
	return "SYS$RFSV";
}

Enum<rfsv::errs> rfsv16::
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
	return E_PSI_GEN_NONE;
}

Enum<rfsv::errs> rfsv16::
fopen(long attr, const char *name, long &handle)
{ 
	bufferStore a;
	char realName[200];
	Enum<rfsv::errs> rv = convertName(name, realName);
	if (rv) return rv;

	// FIXME: anything that calls fopen should NOT do the name
	// conversion - it's just done here. 

	a.addWord(attr & 0xFFFF);
	a.addString(realName);
	a.addByte(0x00); // Needs to be manually Null-Terminated.
	if (!sendCommand(FOPEN, a))
		return E_PSI_FILE_DISC;
  
	Enum<rfsv::errs> res = getResponse(a);
	if (res == 0) {
		handle = (long)a.getWord(0);
		return E_PSI_GEN_NONE;
	}
	return res;
}

// internal
Enum<rfsv::errs> rfsv16::
mktemp(long *handle, char *tmpname)
{
	bufferStore a;

	// FIXME: anything that calls fopen should NOT do the name
	// conversion - it's just done here. 

	a.addWord(P_FUNIQUE);
	a.addString("TMP");
	a.addByte(0x00); // Needs to be manually Null-Terminated.
	if (!sendCommand(OPENUNIQUE, a))
		return E_PSI_FILE_DISC;
  
	Enum<rfsv::errs> res = getResponse(a);
	if (res == E_PSI_GEN_NONE) {
		*handle = a.getWord(0);
		strcpy(tmpname, a.getString(2));
		return res;
	}
	return res;
}

// internal and external
Enum<rfsv::errs> rfsv16::
fcreatefile(long attr, const char *name, long &handle)
{
	return fopen(attr | P_FCREATE, name, handle);
}

// this is internal - not used by plpnfsd, unlike fcreatefile
Enum<rfsv::errs> rfsv16::
freplacefile(long attr, const char *name, long &handle)
{
	return fopen(attr | P_FREPLACE, name, handle);
}

// internal
Enum<rfsv::errs> rfsv16::
fopendir(long attr, const char *name, long &handle)
{
cerr << "rfsv16::fopendir ***" << endl;
	return E_PSI_GEN_NONE;
}

Enum<rfsv::errs> rfsv16::
fclose(long fileHandle)
{
	bufferStore a;
	a.addWord(fileHandle & 0xFFFF);
	if (!sendCommand(FCLOSE, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

Enum<rfsv::errs> rfsv16::
dir(const char *dirName, bufferArray * files)
{
	long fileHandle;
	Enum<rfsv::errs> res = fopen(P_FDIR, dirName, fileHandle);

	if (res != E_PSI_GEN_NONE)
		return res;

	while (1) {
		bufferStore a;
		a.addWord(fileHandle & 0xFFFF);
		if (!sendCommand(FDIRREAD, a))
			return E_PSI_FILE_DISC;
		res = getResponse(a);
		if (res != E_PSI_GEN_NONE)
			break;
		a.discardFirstBytes(2); // Don't know what these mean!
		while (a.getLen() > 16) {
			int version = a.getWord(0);
			if (version != 2) {
				cerr << "dir: not version 2" << endl;
				fclose(fileHandle);
				return E_PSI_GEN_FAIL;
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
			files->append(temp);
		}
	}
	if (res == E_PSI_FILE_EOF)
		res = E_PSI_GEN_NONE;
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

Enum<rfsv::errs> rfsv16::
fgetmtime(const char *name, long *mtime)
{
cerr << "rfsv16::fgetmtime" << endl;
	// NB: fgetattr, fgeteattr is almost identical...
	bufferStore a;
	char realName[200];
	Enum<rfsv::errs> rv = convertName(name, realName);
	if (rv) return rv;
	a.addString(realName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	if (!sendCommand(FINFO, a))
		return E_PSI_FILE_DISC;
  
	Enum<rfsv::errs> res = getResponse(a);
	if (res != E_PSI_GEN_NONE) {
		cerr << "fgetmtime: Error " << res << " on file " << name << endl;	    
		return res;
	}
	else if (a.getLen() == 16) {
		*mtime = a.getDWord(8);
		return res;
	}
	cerr << "fgetmtime: Unknown response (" << name << ") " << a <<endl;
	return E_PSI_GEN_FAIL;
}

Enum<rfsv::errs> rfsv16::
fsetmtime(const char *name, long mtime)
{
cerr << "rfsv16::fsetmtime ***" << endl;
	// I don't think there's a protocol frame that allows us to set the
	// modification time. SFDATE allows setting of creation time...
	return E_PSI_NOT_SIBO;
}

Enum<rfsv::errs> rfsv16::
fgetattr(const char *name, long *attr)
{
	// NB: fgetmtime, fgeteattr are almost identical...
	bufferStore a;
	char realName[200];
	Enum<rfsv::errs> rv = convertName(name, realName);
	if (rv) return rv;
	a.addString(realName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	if (!sendCommand(FINFO, a))
		return E_PSI_FILE_DISC;
  
	Enum<rfsv::errs> res = getResponse(a);
	if (res != 0) {
		cerr << "fgetattr: Error " << res << " on file " << name << endl;	    
		return res;
	}
	else if (a.getLen() == 16) {
		*attr = (long)(a.getWord(2));
		return res;
	}
	cerr << "fgetattr: Unknown response (" << name << ") " << a <<endl;
	return E_PSI_GEN_FAIL;
}

Enum<rfsv::errs> rfsv16::
fgeteattr(const char *name, long *attr, long *size, long *time)
{
	bufferStore a;
	char realName[200];
	Enum<rfsv::errs> rv = convertName(name, realName);
	if (rv) return rv;
	a.addString(realName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	if (!sendCommand(FINFO, a))
		return E_PSI_FILE_DISC;
  
	Enum<rfsv::errs> res = getResponse(a);
	if (res != 0) {
		cerr << "fgeteattr: Error " << res << " on file " << name << endl;
		return res;
	}
	else if (a.getLen() == 16) {
		*attr = (long)(a.getWord(2));
		*size = a.getDWord(4);
		*time = a.getDWord(8);
		return res;
	}
	cerr << "fgeteattr: Unknown response (" << name << ") " << a <<endl;
	return E_PSI_GEN_FAIL;
}

Enum<rfsv::errs> rfsv16::
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

Enum<rfsv::errs> rfsv16::
dircount(const char *name, long *count)
{
	long fileHandle;
	Enum<rfsv::errs> res;
	*count = 0;

	res = fopen(P_FDIR, name, fileHandle);
	if (res != E_PSI_GEN_NONE)
		return res;

	while (1) {
		bufferStore a;
		a.addWord(fileHandle & 0xFFFF);
		if (!sendCommand(FDIRREAD, a))
			return E_PSI_FILE_DISC;
		res = getResponse(a);
		if (res != E_PSI_GEN_NONE)
			break;
		a.discardFirstBytes(2); // Don't know what these mean!
		while (a.getLen() > 16) {
			int version = a.getWord(0);
			if (version != 2) {
				cerr << "dir: not version 2" << endl;
				fclose(fileHandle);
				return E_PSI_GEN_FAIL;
			}
			// int status = a.getWord(2);
			// long size = a.getDWord(4);
			// long date = a.getDWord(8);
			const char *s = a.getString(16);
			a.discardFirstBytes(17+strlen(s));
			(*count)++;
		}
	}
	if (res == E_PSI_FILE_EOF)
		res = E_PSI_GEN_NONE;
	fclose(fileHandle);
	return res;
}

Enum<rfsv::errs> rfsv16::
devlist(long *devbits)
{
	Enum<rfsv::errs> res;
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
	if (res != E_PSI_GEN_NONE)
		return res;

	// Find the drive to FOPEN
	char name[4] = { 'x', ':', '\\', '\0' } ;
	a.discardFirstBytes(6); // Result, fsys, dev, path, file, file, ending, flag
	/* This leaves R E M : : M : \ */
	name[0] = (char) a.getByte(5); // the M
	res = fopen(P_FDEVICE, name, fileHandle);
	if (res != E_PSI_GEN_NONE) 
		return status;

	while (1) {
		bufferStore a;
		a.init();
		a.addWord(fileHandle & 0xFFFF);
		if (!sendCommand(FDEVICEREAD, a))
			return E_PSI_FILE_DISC;
		res = getResponse(a);
		if (res)
			break;
		int version = a.getWord(0);
		if (version != 2) {
			cerr << "devlist: not version 2" << endl;
			fclose(fileHandle);
			return E_PSI_GEN_FAIL; // FIXME
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
	if (res == E_PSI_FILE_EOF)
		res = E_PSI_GEN_NONE;
	fclose(fileHandle);
	return res;
}

char *rfsv16::
devinfo(int devnum, long *vfree, long *vtotal, long *vattr,
	long *vuniqueid)
{
	bufferStore a;
	long res;
	// long fileHandle;

	// Again, this is taken from an exchange between PsiWin and a 3c.
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
	int type = a.getWord(2);
	// int changeable = a.getWord(4);
	long size = a.getDWord(6);
	long free = a.getDWord(10);
	// const char *volume = a.getString(14);
	// int battery = a.getWord(30);
	// const char *devicename = a.getString(62);
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
	if (status == E_PSI_FILE_DISC) {
		reconnect();
		if (status == E_PSI_FILE_DISC)
			return FALSE;
	}
	
	bool result;
	bufferStore a;
	a.addWord(cc);
	a.addWord(data.getLen());
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


Enum<rfsv::errs> rfsv16::
getResponse(bufferStore & data)
{
	// getWord(2) is the size field
	// which is the body of the response not counting the command (002a) and
	// the size word.
	if (skt->getBufferStore(data) != 1) {
		cerr << "rfsv16::getResponse: duff response. "
			"getBufferStore failed." << endl;
	} else if (data.getWord(0) == 0x2a &&
	    data.getWord(2) == data.getLen()-4) {
		Enum<errs> ret = (enum errs)data.getWord(4);
		data.discardFirstBytes(6);
		return ret;
	} else {
	cerr << "rfsv16::getResponse: duff response. Size field:" <<
			data.getWord(2) << " Frame size:" <<
			data.getLen()-4 << " Result field:" <<
			data.getWord(4) << endl;
	}
	status = E_PSI_FILE_DISC;
	return status;
}

long rfsv16::
fread(long handle, unsigned char *buf, long len)
{
	long res;
	long count = 0;

	while (count < len) {
	bufferStore a;

		// Read in blocks of 291 bytes; the maximum payload for
		// an RFSV frame. ( As seen in traces ) - this isn't optimal:
		// RFSV can handle fragmentation of frames, where only the
		// first FREAD RESPONSE frame has a RESPONSE (00 2A), SIZE
		// and RESULT field. Every subsequent frame
	// just has data, 297 bytes (or less) of it.
		a.addWord(handle);
		a.addWord((len - count) > RFSV16_MAXDATALEN
			  ? RFSV16_MAXDATALEN
			  : (len - count));
		sendCommand(FREAD, a);
		res = getResponse(a);

		// The rest of the code treats a 0 return from here
		// as meaning EOF, so we'll arrange for that to happen.
		if (res == E_PSI_FILE_EOF)
			return count;
		else if (res < 0)
			return res;

		res = a.getLen();
		memcpy(buf, a.getString(), res);
		count += res;
		buf += res;
		}
	return count;
}

long rfsv16::
fwrite(long handle, unsigned char *buf, long len)
{
	long res;
	long count = 0;

	while (count < len) {
		bufferStore a;
		int nbytes;

		// Write in blocks of 291 bytes; the maximum payload for
		// an RFSV frame. ( As seen in traces ) - this isn't optimal:
		// RFSV can handle fragmentation of frames, where only the
		// first FREAD RESPONSE frame has a RESPONSE (00 2A), SIZE
		// and RESULT field. Every subsequent frame
		// just has data, 297 bytes (or less) of it.
		nbytes = (len - count) > RFSV16_MAXDATALEN
		    ? RFSV16_MAXDATALEN
		    : (len - count);
		a.addWord(handle);
		a.addBytes(buf, nbytes);
		sendCommand(FWRITE, a);
		res = getResponse(a);
		if (res != 0)
			return res;
		
		count += nbytes;
		buf += nbytes;
	}
	return count;
}

Enum<rfsv::errs> rfsv16::
copyFromPsion(const char *from, const char *to, cpCallback_t cb)
{
	long handle;
	Enum<rfsv::errs> res;
	long len;

	if ((res = fopen(P_FSHARE | P_FSTREAM, from, handle)) != E_PSI_GEN_NONE)
		return res;
	ofstream op(to);
	if (!op) {
		fclose(handle);
		return E_PSI_GEN_FAIL;
	}
	do {
		unsigned char buf[RFSV_SENDLEN];
		if ((len = fread(handle, buf, sizeof(buf))) > 0)
			op.write(buf, len);
		else
			res = (enum rfsv::errs)len;
		if (cb) {
			if (!cb(len)) {
				res = E_PSI_FILE_CANCEL;
				break;
			}
		}
	} while (res > 0);

	fclose(handle);
	op.close();
	if (res == E_PSI_FILE_EOF)
		res = E_PSI_GEN_NONE;
	return res;
}

Enum<rfsv::errs> rfsv16::
copyToPsion(const char *from, const char *to, cpCallback_t cb)
{
	long handle;
	long len = 0;
	Enum<rfsv::errs> res;

	ifstream ip(from);
	if (!ip)
		return E_PSI_FILE_NXIST;
	res = fcreatefile(P_FSTREAM | P_FUPDATE, to, handle);
	if (res != E_PSI_GEN_NONE) {
		res = freplacefile(P_FSTREAM | P_FUPDATE, to, handle);
		if (res != E_PSI_GEN_NONE)
			return res;
	}
	unsigned char *buff = new unsigned char[RFSV_SENDLEN];
	while (res >= 0 && ip && !ip.eof()) {
		ip.read(buff, RFSV_SENDLEN);
		len = fwrite(handle, buff, ip.gcount());
		if (len <= 0)
			res = (enum rfsv::errs)len;
		if (cb)
			if (!cb(len)) {
				res = E_PSI_FILE_CANCEL;
			}
	}

	delete[]buff;
	fclose(handle);
	ip.close();
	return E_PSI_GEN_NONE;
}

Enum<rfsv::errs> rfsv16::
fsetsize(long handle, long size)
{
	bufferStore a;
	a.addWord(handle);
	a.addDWord(size);
	if (!sendCommand(FSETEOF, a))
		return E_PSI_FILE_DISC;
	return getResponse(a);
}

/*
 * Unix-like implementation off fseek with one
 * exception: If seeking beyond eof, the gap
 * contains garbage instead of zeroes.
 */
long rfsv16::
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
		a.init();
		a.addWord(handle);
		a.addDWord(0);
		a.addWord(PSI_SEEK_CUR);
		if (!sendCommand(FSEEK, a))
			return E_PSI_FILE_DISC;
		if ((res = getResponse(a)) != 0)
			return res;
		savpos = a.getDWord(0);
		if (pos == 0)
			return savpos;
	}
	if ((mode == PSI_SEEK_END) && (pos >= 0)) {
		/* get and save end position */
		a.init();
		a.addWord(handle);
		a.addDWord(0);
		a.addWord(PSI_SEEK_END);
		if (!sendCommand(FSEEK, a))
			return E_PSI_FILE_DISC;
		if ((res = getResponse(a)) != 0)
			return res;
		savpos = a.getDWord(0);
		if (pos == 0)
			return savpos;
	}
	/* Now the real seek */
	a.addWord(handle);
	a.addDWord(pos);
	a.addWord(mode);
	if (!sendCommand(FSEEK, a))
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
		res = fsetsize(handle, calcpos);
		if (res != 0)
			return res;
		a.init();
		a.addWord(handle);
		a.addDWord(calcpos);
		a.addWord(PSI_SEEK_SET);
		if (!sendCommand(FSEEK, a))
			return E_PSI_FILE_DISC;
		if ((res = getResponse(a)) != 0)
			return res;
		realpos = a.getDWord(0);
	}
	return realpos;
}

Enum<rfsv::errs> rfsv16::
mkdir(const char* dirName)
{
	char realName[200];
	Enum<rfsv::errs> res = convertName(dirName, realName);
	if (res != E_PSI_GEN_NONE) return res;
	bufferStore a;
	a.addString(realName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	sendCommand(MKDIR, a);
	res = getResponse(a);
	if (res == E_PSI_GEN_NONE) {
		// Correct response
		return res;
	}
	cerr << "Unknown response from mkdir "<< res <<endl;
	return E_PSI_GEN_FAIL;
}

Enum<rfsv::errs> rfsv16::
rmdir(const char *dirName)
{
	// There doesn't seem to be an RMDIR command, but DELETE works. We
	// should probably check to see if the file is a directory first!
	return remove(dirName);
}

Enum<rfsv::errs> rfsv16::
rename(const char *oldName, const char *newName)
{
cerr << "rfsv16::rename ***" << endl;
	char realOldName[200];
	Enum<rfsv::errs> res = convertName(oldName, realOldName);
	if (res != E_PSI_GEN_NONE) return res;
	char realNewName[200];
	res = convertName(newName, realNewName);
	if (res != E_PSI_GEN_NONE) return res;
	bufferStore a;
	a.addString(realOldName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	a.addString(realNewName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	sendCommand(RENAME, a);
	res = getResponse(a);
	if (res == E_PSI_GEN_NONE) {
		// Correct response
		return res;
	}
	cerr << "Unknown response from rename "<< res <<endl;
	return E_PSI_GEN_FAIL;
}

Enum<rfsv::errs> rfsv16::
remove(const char* psionName)
{
	char realName[200];
	Enum<rfsv::errs> res = convertName(psionName, realName);
	if (res != E_PSI_GEN_NONE) return res;
	bufferStore a;
	a.addString(realName);
	a.addByte(0x00); 	// needs to be null-terminated, 
				// and this needs sending in the length word.
	sendCommand(DELETE, a);
	res = getResponse(a);
	if (res == E_PSI_GEN_NONE) {
		// Correct response
		return res;
	}
	cerr << "Unknown response from delete "<< res <<endl;
	return E_PSI_GEN_FAIL;
}

/*
 * Translate SIBO attributes to standard attributes.
 */
long rfsv16::
attr2std(long attr)
{
	long res = 0;

	// Common attributes
	if (!(attr & P_FAWRITE))
		res |= PSI_A_RDONLY;
	if (attr & P_FAHIDDEN)
		res |= PSI_A_HIDDEN;
	if (attr & P_FASYSTEM)
		res |= PSI_A_SYSTEM;
	if (attr & P_FADIR)
		res |= PSI_A_DIR;
	if (attr & P_FAMOD)
		res |= PSI_A_ARCHIVE;
	if (attr & P_FAVOLUME)
		res |= PSI_A_VOLUME;

	// SIBO-specific
	if (attr & P_FAREAD)
		res |= PSI_A_READ;
	if (attr & P_FAEXEC)
		res |= PSI_A_EXEC;
	if (attr & P_FASTREAM)
		res |= PSI_A_STREAM;
	if (attr & P_FATEXT)
		res |= PSI_A_TEXT;

	// Do what we can for EPOC
	res |= PSI_A_NORMAL;

	return res;
}

/*
 * Translate standard attributes to SIBO attributes.
 */
long rfsv16::
std2attr(long attr)
{
	long res = 0;

	// Common attributes
	if (!(attr & PSI_A_RDONLY))
		res |= P_FAWRITE;
	if (attr & PSI_A_HIDDEN)
		res |= P_FAHIDDEN;
	if (attr & PSI_A_SYSTEM)
		res |= P_FASYSTEM;
	if (attr & PSI_A_DIR)
		res |= P_FADIR;
	if (attr & PSI_A_ARCHIVE)
		res |= P_FAMOD;
	if (attr & PSI_A_VOLUME)
		res |= P_FAVOLUME;

	// SIBO-specific
	if (attr & PSI_A_READ)
		res |= P_FAREAD;
	if (attr & PSI_A_EXEC)
		res |= P_FAEXEC;
	if (attr & PSI_A_STREAM)
		res |= P_FASTREAM;
	if (attr & PSI_A_TEXT)
		res |= P_FATEXT;

	return res;
}


