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
#include <config.h>
#endif

#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string>
#include <fstream>
#include <iomanip>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>

extern "C" {
#include "mp_serial.h"
}
#include "packet.h"
#include "link.h"

#define BUFLEN 4096 // Must be a power of 2
#define BUFMASK (BUFLEN-1)
#define hasSpace(dir) (((dir##Write + 1) & BUFMASK) != dir##Read)
#define hasData(dir) (dir##Write != dir##Read)
#define inca(idx,amount) do { \
    idx = (idx + amount) & BUFMASK; \
} while (0)
#define inc1(idx) inca(idx, 1)
#define normalize(idx) do { idx &= BUFMASK; } while (0)

static unsigned short pumpverbose = 0;

extern "C" {
/**
 * Signal handler does nothing. It just exists
 * for having the select() below return an
 * interrupted system call.
 */
static void usr1handler(int sig)
{
    signal(SIGUSR1, usr1handler);
}


static void *pump_run(void *arg)
{
    packet *p = (packet *)arg;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    while (1) {
	if (p->fd != -1) {
	    fd_set r_set;
	    fd_set w_set;
	    int res;
	    int count;

	    FD_ZERO(&r_set);
	    w_set = r_set;
	    if (hasSpace(p->in))
		FD_SET(p->fd, &r_set);
	    if (hasData(p->out))
		FD_SET(p->fd, &w_set);
	    res = select(p->fd+1, &r_set, &w_set, NULL, NULL);
	    switch (res) {
		case 0:
		    break;
		case -1:
		    break;
		default:
		    if (FD_ISSET(p->fd, &w_set)) {
			count = p->outWrite - p->outRead;
			if (count < 0)
			    count = (BUFLEN - p->outRead);
			res = write(p->fd, &p->outBuffer[p->outRead], count);
			if (res > 0) {
			    if (pumpverbose & PKT_DEBUG_DUMP) {
				int i;
				printf("pump: wrote %d bytes: (", res);
				for (i = 0; i<res; i++)
				    printf("%02x ",
					   p->outBuffer[p->outRead + i]);
				printf(")\n");
			    }
			    int hadSpace = hasSpace(p->out);
			    inca(p->outRead, res);
			    if (!hadSpace)
				    pthread_kill(p->thisThread, SIGUSR1);
			}
		    }
		    if (FD_ISSET(p->fd, &r_set)) {
			count = p->inRead - p->inWrite;
			if (count <= 0)
			    count = (BUFLEN - p->inWrite);
			res = read(p->fd, &p->inBuffer[p->inWrite], count);
			if (res > 0) {
			    if (pumpverbose & PKT_DEBUG_DUMP) {
				int i;
				printf("pump: read %d bytes: (", res);
				for (i = 0; i<res; i++)
				    printf("%02x ", p->inBuffer[p->inWrite + i]);
				printf(")\n");
			    }
			    inca(p->inWrite, res);
			    p->findSync();
			}
		    } else {
			if (hasData(p->in))
			    p->findSync();
		    }
		    break;
	    }
	}
    }
}

//static pthread_mutex_t outMutex;
//static pthread_mutex_t inMutex;
};

packet::
packet(const char *fname, int _baud, Link *_link, unsigned short _verbose)
{
    verbose = pumpverbose = _verbose;
    devname = strdup(fname);
    assert(devname);
    baud = _baud;
    theLINK = _link;
    isEPOC = false;

    // Initialize CRC table
    crc_table[0] = 0;
    for (int i = 0; i < 128; i++) {
	unsigned int carry = crc_table[i] & 0x8000;
	unsigned int tmp = (crc_table[i] << 1) & 0xffff;
	crc_table[i * 2 + (carry ? 0 : 1)] = tmp ^ 0x1021;
	crc_table[i * 2 + (carry ? 1 : 0)] = tmp;
    }

    inRead = inWrite = outRead = outWrite = 0;
    inBuffer = new unsigned char[BUFLEN + 1];
    outBuffer = new unsigned char[BUFLEN + 1];
    assert(inBuffer);
    assert(outBuffer);

    esc = false;
    lastFatal = false;
    serialStatus = -1;
    lastSYN = startPkt = -1;
    crcIn = crcOut = 0;

    thisThread = pthread_self();
    fd = init_serial(devname, baud, 0);
    if (fd == -1)
	lastFatal = true;
    else {
//	pthread_mutex_init(&inMutex, NULL);
//	pthread_mutex_init(&outMutex, NULL);
	signal(SIGUSR1, usr1handler);
	pthread_create(&datapump, NULL, pump_run, this);
    }
}

packet::
~packet()
{
    if (fd != -1) {
	pthread_cancel(datapump);
	ser_exit(fd);
    }
    fd = -1;
    delete []inBuffer;
    delete []outBuffer;
    free(devname);
}

void packet::
reset()
{
    if (verbose & PKT_DEBUG_LOG)
	cout << "resetting serial connection" << endl;
    if (fd != -1) {
	pthread_cancel(datapump);
	ser_exit(fd);
	fd = -1;
    }
    usleep(100000);
    inRead = inWrite = outRead = outWrite = 0;
    esc = false;
    lastFatal = false;
    serialStatus = -1;
    lastSYN = startPkt = -1;
    crcIn = crcOut = 0;
    fd = init_serial(devname, baud, 0);
    if (fd != -1)
	lastFatal = false;
    else {
//	pthread_mutex_init(&inMutex, NULL);
//	pthread_mutex_init(&outMutex, NULL);
	pthread_create(&datapump, NULL, pump_run, this);
    }
    if (verbose & PKT_DEBUG_LOG)
	cout << "serial connection reset, fd=" << fd << endl;
}

short int packet::
getVerbose()
{
    return verbose;
}

void packet::
setVerbose(short int _verbose)
{
    verbose = pumpverbose = _verbose;
}

void packet::
setEpoc(bool _epoc)
{
    isEPOC = _epoc;
}

void packet::
send(bufferStore &b)
{
    opByte(0x16);
    opByte(0x10);
    opByte(0x02);

    crcOut = 0;
    long len = b.getLen();

    if (verbose & PKT_DEBUG_LOG) {
	cout << "packet: >> ";
	if (verbose & PKT_DEBUG_DUMP)
	    cout << b;
	else
	    cout << " len=" << dec << len;
	cout << endl;
    }

    for (int i = 0; i < len; i++) {
	unsigned char c = b.getByte(i);
	switch (c) {
	    case 0x03:
		if (isEPOC) {
		    opByte(0x10);
		    opByte(0x04);
		    addToCrc(0x03, &crcOut);
		} else
		    opCByte(c, &crcOut);
		break;
	    case 0x10:
		opByte(0x10);
		// fall thru
	    default:
		opCByte(c, &crcOut);
	}
    }
    opByte(0x10);
    opByte(0x03);
    opByte(crcOut >> 8);
    opByte(crcOut & 0xff);
    realWrite();
}

void packet::
opByte(unsigned char a)
{
    if (!hasSpace(out))
	realWrite();
    outBuffer[outWrite] = a;
    inc1(outWrite);
}

void packet::
opCByte(unsigned char a, unsigned short *crc)
{
    addToCrc(a, crc);
    if (!hasSpace(out))
	realWrite();
    outBuffer[outWrite] = a;
    inc1(outWrite);
}

void packet::
realWrite()
{
    pthread_kill(datapump, SIGUSR1);
    while (!hasSpace(out)) {
	sigset_t sigs;
	int dummy;
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGUSR1);
	sigwait(&sigs, &dummy);
    }
}

void packet::
findSync()
{
    int inw = inWrite;
    int p;

 outerLoop:
    p = (lastSYN >= 0) ? lastSYN : inRead;
    if (startPkt < 0) {
	while (p != inw) {
	    normalize(p);
	    if (inBuffer[p++] != 0x16)
		continue;
	    lastSYN = p - 1;
	    normalize(p);
	    if (inBuffer[p++] != 0x10)
		continue;
	    normalize(p);
	    if (inBuffer[p++] != 0x02)
		continue;
	    normalize(p);
	    lastSYN = startPkt = p;
	    crcIn = inCRCstate = 0;
	    rcv.init();
	    esc = false;
	    break;
	}
    }
    if (startPkt >= 0) {
	while (p != inw) {
	    unsigned char c = inBuffer[p];
	    switch (inCRCstate) {
		case 0:
		    if (esc) {
			esc = false;
			switch (c) {
			    case 0x03:
				inCRCstate = 1;
				break;
			    case 0x04:
				addToCrc(0x03, &crcIn);
				rcv.addByte(0x03);
				break;
			    default:
				addToCrc(c, &crcIn);
				rcv.addByte(c);
				break;
			}
		    } else {
			if (c == 0x10)
			    esc = true;
			else {
			    addToCrc(c, &crcIn);
			    rcv.addByte(c);
			}
		    }
		    break;
		case 1:
		    receivedCRC = c;
		    receivedCRC <<= 8;
		    inCRCstate = 2;
		    break;
		case 2:
		    receivedCRC |= c;
		    inc1(p);
		    inRead = p;
		    startPkt = lastSYN = -1;
		    inCRCstate = 0;
		    if (receivedCRC != crcIn) {
			if (verbose & PKT_DEBUG_LOG)
			    cout << "packet: BAD CRC" << endl;
		    } else {
			// inQueue += rcv;
			if (verbose & PKT_DEBUG_LOG) {
			    cout << "packet: << ";
			    if (verbose & PKT_DEBUG_DUMP)
				cout << rcv;
			    else
				cout << "len=" << dec << rcv.getLen();
			    cout << endl;
			}
			theLINK->receive(rcv);
		    }
		    rcv.init();
		    if (hasData(out))
			return;
		    goto outerLoop;
	    }
	    inc1(p);
	}
	lastSYN = p;
    }
}

bool packet::
linkFailed()
{
    int arg;
    int res;
    bool failed = false;

    if (lastFatal)
	reset();
    res = ioctl(fd, TIOCMGET, &arg);
    if (res < 0)
	lastFatal = true;
    if ((serialStatus == -1) || (arg != serialStatus)) {
	if (verbose & PKT_DEBUG_HANDSHAKE)
	    cout << "packet: < DTR:" << ((arg & TIOCM_DTR)?1:0)
		 << " RTS:" << ((arg & TIOCM_RTS)?1:0)
		 << " DCD:" << ((arg & TIOCM_CAR)?1:0)
		 << " DSR:" << ((arg & TIOCM_DSR)?1:0)
		 << " CTS:" << ((arg & TIOCM_CTS)?1:0) << endl;
	if (!((arg & TIOCM_RTS) && (arg & TIOCM_DTR))) {
	    arg |= (TIOCM_DTR | TIOCM_RTS);
	    res = ioctl(fd, TIOCMSET, &arg);
	    if (res < 0)
		lastFatal = true;
	    if (verbose & PKT_DEBUG_HANDSHAKE)
		cout << "packet: > DTR:" << ((arg & TIOCM_DTR)?1:0)
		     << " RTS:" << ((arg & TIOCM_RTS)?1:0)
		     << " DCD:" << ((arg & TIOCM_CAR)?1:0)
		     << " DSR:" << ((arg & TIOCM_DSR)?1:0)
		     << " CTS:" << ((arg & TIOCM_CTS)?1:0) << endl;
	}
	serialStatus = arg;
    }
    if (((arg & TIOCM_CTS) == 0)
#ifndef sun
	|| ((arg & TIOCM_DSR) == 0)
#endif
	) {
	// eat possible junk on line
	//while (read(fd, &res, sizeof(res)) > 0)
	//	;
	failed = true;
    }
    if ((verbose & PKT_DEBUG_LOG) && lastFatal)
	cout << "packet: linkFATAL\n";
    if ((verbose & PKT_DEBUG_LOG) && failed)
	cout << "packet: linkFAILED\n";
    return lastFatal || failed;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
