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
#include <string.h>
#include <time.h>

#include "bool.h"
#include "defs.h"
#include "ncp.h"
#include "linkchan.h"
#include "bufferstore.h"
#include "link.h"
#include "bufferarray.h"

#define NCP_SENDLEN 250

ncp::ncp(const char *fname, int baud, IOWatch & iow)
{
	l = new link(fname, baud, iow, LINK_LAYER_DIAGNOSTICS);
	gotLinkChan = false;
	failed = false;

	// init channels
	for (int i = 0; i < 8; i++)
		channelPtr[i] = NULL;
}

ncp::~ncp()
{
	delete l;
}

void ncp::
poll()
{
	bufferArray res(l->poll());
	if (!res.empty()) {
		do {
			bufferStore s = res.popBuffer();
			if (s.getLen() > 1) {
				int channel = s.getByte(0);
				s.discardFirstBytes(1);
				if (channel == 0) {
					decodeControlMessage(s);
				} else {
					/* int remChan = */ s.getByte(0);
					int allData = s.getByte(1);
					s.discardFirstBytes(2);
					if (channelPtr[channel] == NULL) {
						cerr << "Got message for unknown channel\n";
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
			} else {
				cerr << "Got null message\n";
			}
		} while (!res.empty());
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
	cout << "put " << ctrlMsgName(t) << endl;
	l->send(open);
}

void ncp::
decodeControlMessage(bufferStore & buff)
{
	int remoteChan = buff.getByte(0);
	interControllerMessageType imt = (interControllerMessageType) buff.getByte(1);
	buff.discardFirstBytes(2);
	cout << "got " << ctrlMsgName(imt) << " " << remoteChan << " ";
	switch (imt) {
		case NCON_MSG_DATA_XOFF:
			cout << remoteChan << "  " << buff << endl;
			break;
		case NCON_MSG_DATA_XON:
			cout << buff << endl;
			break;
		case NCON_MSG_CONNECT_TO_SERVER:{
				cout << buff << endl;
				int localChan;
				bufferStore b;

				// Ack with connect response
				localChan = getFirstUnusedChan();
				b.addByte(remoteChan);
				b.addByte(0x0);
				controlChannel(localChan, NCON_MSG_CONNECT_RESPONSE, b);

				if (!strcmp(buff.getString(0), "LINK.*")) {
					if (gotLinkChan)
						failed = true;
					cout << "Accepted link channel" << endl;
					channelPtr[localChan] = new linkChan(this);
					channelPtr[localChan]->setNcpChannel(localChan);
					channelPtr[localChan]->ncpConnectAck();
					gotLinkChan = true;
				} else {
					cout << "Disconnecting channel" << endl;
					bufferStore b;
					b.addByte(remoteChan);
					controlChannel(localChan, NCON_MSG_CHANNEL_DISCONNECT, b);
				}
			}
			break;
		case NCON_MSG_CONNECT_RESPONSE:{
				int forChan = buff.getByte(0);
				cout << "for channel " << forChan << " status ";
				if (buff.getByte(1) == 0) {
					cout << "OK" << endl;
					if (channelPtr[forChan]) {
						remoteChanList[forChan] = remoteChan;
						channelPtr[forChan]->ncpConnectAck();
					} else {
						cerr << "Got message for unknown channel\n";
					}
				} else {
					cout << "Unknown status " << (int) buff.getByte(1) << endl;
					channelPtr[forChan]->ncpConnectTerminate();
				}
			}
			break;
		case NCON_MSG_CHANNEL_CLOSED:
			cout << buff << endl;
			break;
		case NCON_MSG_NCP_INFO:
			if (buff.getByte(0) == 6) {
				cout << buff << endl;
				{
					// Send time info
					bufferStore b;
					b.addByte(6);
					b.addDWord(0);
					controlChannel(0, NCON_MSG_NCP_INFO, b);
				}
			} else
				cout << "ALERT!!!! Protocol-Version is NOT 6!! (No Series 5?)!" << endl;
			break;
		case NCON_MSG_CHANNEL_DISCONNECT:
			cout << "channel " << (int) buff.getByte(0) << endl;
			disconnect(buff.getByte(0));
			break;
		case NCON_MSG_NCP_END:
			cout << buff << endl;
			break;
		default:
			cout << endl;
	}
}

int ncp::
getFirstUnusedChan()
{
	for (int cNum = 1; cNum < 8; cNum++) {
		if (channelPtr[cNum] == NULL) {
			return cNum;
		}
	}
	return 0;
}

int ncp::
connect(channel * ch)
{
	// look for first unused chan
	int cNum = getFirstUnusedChan();
	if (cNum > 0) {
		channelPtr[cNum] = ch;
		ch->setNcpChannel(cNum);
		bufferStore b;
		b.addString(ch->getNcpConnectName());
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
	channelPtr[channel]->terminateWhenAsked();
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
	return gotLinkChan;
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
