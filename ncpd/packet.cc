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


#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <fstream.h>
#include <iomanip.h>
#include <errno.h>

extern "C" {
#include "mp_serial.h"
}
#include "bool.h"
#include "bufferstore.h"
#include "packet.h"
#include "iowatch.h"

#define BUFFERLEN 2000

packet::packet(const char *fname, int _baud, IOWatch & _iow, bool _verbose):
iow(_iow)
{
	verbose = _verbose;
	devname = strdup(fname);
	baud = _baud;
	inPtr = inBuffer = new unsigned char[BUFFERLEN + 1];
	outPtr = outBuffer = new unsigned char[BUFFERLEN + 1];
	inLen = outLen = termLen = 0;
	esc = false;
	crcIn = crcOut = 0;

	fd = init_serial(devname, baud, 0);
	iow.addIO(fd);
}

packet::~packet()
{
	iow.remIO(fd);
	ser_exit(fd);
	usleep(100000);

	delete[]inBuffer;
	delete[]outBuffer;
	free(devname);
}

void packet::
send(unsigned char type, const bufferStore & b)
{
	if (verbose)
		cout << "packet: send ";
	opByte(0x16);
	opByte(0x10);
	opByte(0x02);

	crcOut = 0;
	opByte(type);
	addToCrc(type, &crcOut);

	long len = b.getLen();
	for (int i = 0; i < len; i++) {
		unsigned char c = b.getByte(i);
		if (c == 0x10)
			opByte(c);
		opByte(c);
		addToCrc(c, &crcOut);
	}

	opByte(0x10);
	opByte(0x03);

	opByte(crcOut >> 8);
	opByte(crcOut & 0xff);
	if (verbose)
		cout << endl;
	realWrite();
}

void packet::
addToCrc(unsigned short c, unsigned short *crc)
{
	c <<= 8;
	for (int i = 0; i < 8; i++) {
		if ((*crc ^ c) & 0x8000)
			*crc = (*crc << 1) ^ 0x1021;
		else
			*crc <<= 1;
		c <<= 1;
	}
}

void packet::
opByte(unsigned char a)
{
	*outPtr++ = a;
	outLen++;
	if (verbose)
		cout << hex << setw(2) << setfill('0') << (int) a << " ";
	if (outLen >= BUFFERLEN)
		realWrite();
}

void packet::
realWrite()
{
	outPtr = outBuffer;
	while (outLen > 0) {
		int r = write(fd, outPtr, outLen);
		if (verbose)
			cout << "realW:" << dec << r << endl;
		if (r > 0) {
			outLen -= r;
			outPtr += r;
		}
	}
	outPtr = outBuffer;
}

bool packet::
get(unsigned char &type, bufferStore & ret)
{
	while (!terminated()) {
		int res = read(fd, inPtr, BUFFERLEN - inLen);
		if (res > 0) {
			if (verbose)
				cout << "packet: rcv " << dec << res << endl;
			inPtr += res;
			inLen += res;
		}
		if (res < 0)
			return false;
		if (inLen >= BUFFERLEN) {
			cerr << "input Overflow!!!!" << endl;
			inLen = 0;
			inPtr = inBuffer;
			return false;
		}
	}
	if (verbose) {
		cout << "packet: get ";
		for (int i = 0; i < termLen; i++)
			cout << hex << setw(2) << setfill('0') << (int) inBuffer[i] << " ";
		cout << endl;
	}
	inLen -= termLen;
	termLen = 0;
	bool crcOk = (endPtr[0] == ((crcIn >> 8) & 0xff) && endPtr[1] == (crcIn & 0xff));
	if (inLen > 0)
		memmove(inBuffer, &endPtr[2], inLen);
	inPtr = inBuffer + inLen;
	if (crcOk) {
		type = rcv.getByte(0);
		ret = rcv;
		ret.discardFirstBytes(1);
		return true;
	} else
		cout << "packet::Warning - bad crc packet " << endl;
	return false;
}

bool packet::
terminated()
{
	unsigned char *p;
	int l;

	if (inLen < 6)
		return false;
	p = inBuffer + termLen;
	if (termLen == 0) {
		if (*p++ != 0x16)
			return false;
		if (*p++ != 0x10)
			return false;
		if (*p++ != 0x02)
			return false;
		esc = false;
		termLen = 3;
		crcIn = 0;
		rcv.init();
	}
	for (l = termLen; l < inLen - 2; p++, l++) {
		if (esc) {
			esc = false;
			if (*p == 0x03) {
				endPtr = p + 1;
				termLen = l + 3;
				return true;
			}
			addToCrc(*p, &crcIn);
			rcv.addByte(*p);
		} else {
			if (*p == 0x10)
				esc = true;
			else {
				addToCrc(*p, &crcIn);
				rcv.addByte(*p);
			}
		}
	}
	termLen = l;
	return false;
}
