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

#include <stream.h>
#include <string>
#include <time.h>

#include "ncp.h"
#include "linkchan.h"
#include <bufferstore.h>
#include "link.h"
#include <bufferarray.h>

#define MAX_CHANNELS_PSION 256
#define MAX_CHANNELS_SIBO  8
#define NCP_SENDLEN 250

ncp::ncp(const char *fname, int baud, IOWatch *iow)
{
    channelPtr = new channel*[MAX_CHANNELS_PSION + 1];
    messageList = new bufferStore[MAX_CHANNELS_PSION + 1];
    remoteChanList = new int[MAX_CHANNELS_PSION + 1];

    l = new link(fname, baud, iow);
    failed = false;
    verbose = 0;

    // until detected on receipt of INFO we use these.
    maxChannels = MAX_CHANNELS_SIBO;
    protocolVersion = PV_SERIES_5;

    // init channels
    for (int i = 0; i < MAX_CHANNELS_PSION; i++)
	channelPtr[i] = NULL;
}

ncp::~ncp()
{
    bufferStore b;
    for (int i = 0; i < maxLinks(); i++) {
	if (channelPtr[i]) {
	    bufferStore b2;
	    b2.addByte(remoteChanList[i]);
	    controlChannel(i, NCON_MSG_CHANNEL_DISCONNECT, b2);
	}
	channelPtr[i] = NULL;
    }
    controlChannel(0, NCON_MSG_NCP_END, b);
    delete l;
    delete [] channelPtr;
    delete [] remoteChanList;
    delete [] messageList;
}

int ncp::
maxLinks() {
    return maxChannels;
}

void ncp::
reset() {
    for (int i = 0; i < maxLinks(); i++) {
	if (channelPtr[i])
	    channelPtr[i]->terminateWhenAsked();
	channelPtr[i] = NULL;
    }
    failed = false;
    lChan = NULL;
    protocolVersion = PV_SERIES_5; // until detected on receipt of INFO
    l->reset();
}

short int ncp::
getVerbose()
{
    return verbose;
}

void ncp::
setVerbose(short int _verbose)
{
    verbose = _verbose;
}

short int ncp::
getLinkVerbose()
{
    return l->getVerbose();
}

void ncp::
setLinkVerbose(short int _verbose)
{
    l->setVerbose(_verbose);
}

short int ncp::
getPktVerbose()
{
    return l->getPktVerbose();
}

void ncp::
setPktVerbose(short int _verbose)
{
    l->setPktVerbose(_verbose);
}

short int ncp::
getProtocolVersion()
{
    return protocolVersion;
}

void ncp::
poll()
{
    bufferArray res(l->poll());
    while (!res.empty()) {
	bufferStore s = res.pop();
	if (s.getLen() > 1) {
	    int channel = s.getByte(0);
	    s.discardFirstBytes(1);
	    if (channel == 0) {
		decodeControlMessage(s);
	    } else {
		int allData = s.getByte(1);
		s.discardFirstBytes(2);
		if (channelPtr[channel] == NULL) {
		    cerr << "ncp: Got message for unknown channel\n";
		} else {
		    messageList[channel].addBuff(s);
		    if (allData == LAST_MESS) {
			channelPtr[channel]->ncpDataCallback(messageList[channel]);
			messageList[channel].init();
		    } else if (allData != NOT_LAST_MESS) {
			cerr << "ncp: bizarre third byte!\n";
		    }
		}
	    }
	} else
	    cerr << "Got null message\n";
    }
}

void ncp::
controlChannel(int chan, enum interControllerMessageType t, bufferStore & command)
{
    bufferStore open;
    open.addByte(0);	// control

    open.addByte(chan);
    open.addByte(t);
    open.addBuff(command);
    if (verbose & NCP_DEBUG_LOG)
	cout << "ncp: >> " << ctrlMsgName(t) << " " << chan << endl;
    l->send(open);
}

void ncp::
decodeControlMessage(bufferStore & buff)
{
    int remoteChan = buff.getByte(0);

    interControllerMessageType imt = (interControllerMessageType)buff.getByte(1);
    buff.discardFirstBytes(2);
    if (verbose & NCP_DEBUG_LOG)
	cout << "ncp: << " << ctrlMsgName(imt) << " " << remoteChan;

    bufferStore b;

    switch (imt) {
	case NCON_MSG_CONNECT_TO_SERVER:
	    if (verbose & NCP_DEBUG_LOG) {
		if (verbose & NCP_DEBUG_DUMP)
		    cout << " [" << buff << "]";
		cout << endl;
	    }

	    int localChan;

	    // Ack with connect response
	    localChan = getFirstUnusedChan();
	    b.addByte(remoteChan);
	    b.addByte(0x0);
	    controlChannel(localChan, NCON_MSG_CONNECT_RESPONSE, b);

	    // NOTE: we don't allow connections from the
	    // Psion to any local "processes" other than
	    // LINK.* - Matt might need to change this for
	    // his NCP<->TCP/IP bridge code...

	    if (!strcmp(buff.getString(0), "LINK.*")) {
		if (lChan)
		    failed = true;
		if (verbose & NCP_DEBUG_LOG)
		    cout << "ncp: Link UP" << endl;
		channelPtr[localChan] = lChan = new linkChan(this);
		lChan->setNcpChannel(localChan);
		lChan->ncpConnectAck();
		lChan->setVerbose(verbose);
	    } else {
		if (verbose & NCP_DEBUG_LOG)
		    cout << "ncp: REJECT connect" << endl;
		bufferStore b;
		b.addByte(remoteChan);
		controlChannel(0, NCON_MSG_CHANNEL_DISCONNECT, b);
	    }
	    break;

	case NCON_MSG_CONNECT_RESPONSE:

	    int forChan;

	    forChan = buff.getByte(0);
	    if (verbose & NCP_DEBUG_LOG)
		cout << " ch=" << forChan << " stat=";
	    if (buff.getByte(1) == 0) {
		if (verbose & NCP_DEBUG_LOG)
		    cout << "OK" << endl;
		if (channelPtr[forChan]) {
		    remoteChanList[forChan] = remoteChan;
		    channelPtr[forChan]->ncpConnectAck();
		} else {
		    if (verbose & NCP_DEBUG_LOG)
			cout << "ncp: message for unknown channel" << endl;
		}
	    } else {
		if (verbose & NCP_DEBUG_LOG)
		    cout << "Unknown " << (int) buff.getByte(1) << endl;
		if (channelPtr[forChan])
		    channelPtr[forChan]->ncpConnectNak();
	    }
	    break;

	case NCON_MSG_NCP_INFO:

	    int ver;

	    ver = buff.getByte(0);
	    // Series 3c returns '3', as does mclink. PsiWin 1.1
	    // returns version 2. We return whatever version we're
	    // sent, which is rather crude, but works for Series 3
	    // and probably 5. If Symbian have changed EPOC Connect
	    // for the Series 5mx/7, this may need to change.
	    //
	    if (ver == PV_SERIES_5 || ver == PV_SERIES_3) {
		bufferStore b;
		protocolVersion = ver;
		if (verbose & NCP_DEBUG_LOG) {
		    if (verbose & NCP_DEBUG_DUMP)
			cout << " [" << buff << "]";
		    cout << endl;
		}
		// Fake NCP version 2 for a Series 3 (behave like PsiWin 1.1)
		if (ver == PV_SERIES_3) {
		    ver = 2;
		} else {
		    // Series 5 supports more channels
		    maxChannels = MAX_CHANNELS_PSION;
		}
		b.addByte(ver);
		// Do we send a time of 0 or a real time?
		// The Psion uses this to determine whether to
		// restart. (See protocol docs for details)
		// b.addDWord(0);
		b.addDWord(time(NULL));
		controlChannel(0, NCON_MSG_NCP_INFO, b);
	    } else
		cout << "ALERT!!!! Unexpected Protocol Version!! (No Series 5/3?)!" << endl;
	    break;

	case NCON_MSG_CHANNEL_DISCONNECT:
	    if (verbose & NCP_DEBUG_LOG)
		cout << " ch=" << (int) buff.getByte(0) << endl;
	    disconnect(buff.getByte(0));
	    l->purgeQueue(remoteChan);
	    break;

	case NCON_MSG_DATA_XOFF:
	case NCON_MSG_DATA_XON:
	case NCON_MSG_CHANNEL_CLOSED:
	case NCON_MSG_NCP_END:
	default:
	    if (verbose & NCP_DEBUG_LOG) {
		if (verbose & NCP_DEBUG_DUMP)
		    cout << " [" << buff << "]";
		cout << endl;
	    }

    }
}

int ncp::
getFirstUnusedChan()
{
    for (int cNum = 1; cNum < maxLinks(); cNum++) {
	if (channelPtr[cNum] == NULL) {
	    if (verbose & NCP_DEBUG_LOG)
		cout << "ncp: getFirstUnusedChan=" << cNum << endl;
	    return cNum;
	}
    }
    return 0;
}

void ncp::
RegisterAck(int chan, const char *name)
{
    if (verbose & NCP_DEBUG_LOG)
	cout << "ncp: RegisterAck: chan=" << chan << endl;
    for (int cNum = 1; cNum < maxLinks(); cNum++) {
	channel *ch = channelPtr[cNum];
	if (ch && ch->getNcpChannel() == chan) {
	    ch->setNcpConnectName(name);
	    ch->ncpRegisterAck();
	    return;
	}
    }
    cerr << "ncp: RegisterAck: no channel to deliver" << endl;
}

void ncp::
Register(channel * ch)
{
    if (lChan) {
	int cNum = ch->getNcpChannel();
	if (cNum == 0)
	    cNum = getFirstUnusedChan();
	if (cNum > 0) {
	    channelPtr[cNum] = ch;
	    ch->setNcpChannel(cNum);
	    lChan->Register(ch);
	} else
	    cerr << "ncp: Out of channels in register" << endl;
    } else
	cerr << "ncp: Register without established lChan" << endl;
}

int ncp::
connect(channel * ch)
{
    // look for first unused chan

    int cNum = ch->getNcpChannel();
    if (cNum == 0)
	cNum = getFirstUnusedChan();
    if (cNum > 0) {
	channelPtr[cNum] = ch;
	ch->setNcpChannel(cNum);
	bufferStore b;
	if (ch->getNcpConnectName())
	    b.addString(ch->getNcpConnectName());
	else {
	    b.addString(ch->getNcpRegisterName());
	    b.addString(".*");
	}
	b.addByte(0);
	controlChannel(cNum, NCON_MSG_CONNECT_TO_SERVER, b);
	return cNum;
    }
    return -1;
}

void ncp::
send(int channel, bufferStore & a)
{
    bool last;
    do {
	last = true;

	if (a.getLen() > NCP_SENDLEN)
	    last = false;

	bufferStore out;
	out.addByte(remoteChanList[channel]);
	out.addByte(channel);

	if (last) {
	    out.addByte(LAST_MESS);
	} else {
	    out.addByte(NOT_LAST_MESS);
	}

	out.addBuff(a, NCP_SENDLEN);
	a.discardFirstBytes(NCP_SENDLEN);
	l->send(out);
    } while (!last);
}

void ncp::
disconnect(int channel)
{
    if (channelPtr[channel] == NULL) {
	cerr << "ncp: Ignored disconnect for unknown channel #" << channel << endl;
	return;
    }
    channelPtr[channel]->terminateWhenAsked();
    if (verbose & NCP_DEBUG_LOG)
	cout << "ncp: disconnect: channel=" << channel << endl;
    channelPtr[channel] = NULL;
    bufferStore b;
    b.addByte(remoteChanList[channel]);
    controlChannel(channel, NCON_MSG_CHANNEL_DISCONNECT, b);
}

bool ncp::
stuffToSend()
{
    return l->stuffToSend();
}

bool ncp::
hasFailed()
{
    if (failed)
	return true;
    return l->hasFailed();
}

bool ncp::
gotLinkChannel()
{
    return (lChan != NULL);
}

char *ncp::
ctrlMsgName(unsigned char msgType)
{
    switch (msgType) {
	case NCON_MSG_DATA_XOFF:
	    return "NCON_MSG_DATA_XOFF";
	case NCON_MSG_DATA_XON:
	    return "NCON_MSG_DATA_XON";
	case NCON_MSG_CONNECT_TO_SERVER:
	    return "NCON_MSG_CONNECT_TO_SERVER";
	case NCON_MSG_CONNECT_RESPONSE:
	    return "NCON_MSG_CONNECT_RESPONSE";
	case NCON_MSG_CHANNEL_CLOSED:
	    return "NCON_MSG_CHANNEL_CLOSED";
	case NCON_MSG_NCP_INFO:
	    return "NCON_MSG_NCP_INFO";
	case NCON_MSG_CHANNEL_DISCONNECT:
	    return "NCON_MSG_CHANNEL_DISCONNECT";
	case NCON_MSG_NCP_END:
	    return "NCON_MSG_NCP_END";
    }
    return "NCON_MSG_UNKNOWN";
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
