// $Id$
//
//  PLP - An implementation of the PSION link protocol
//
//  Copyright (C) 1999  Philip Proudman
//  Modifications for plptools:
//    Copyright (C) 1999 Fritz Elfert <felfert@to.com>
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
#include "stdio.h"
#include "string.h"
#include "malloc.h"

#include "bool.h"
#include "socketchan.h"
#include "ncp.h"
#include "ppsocket.h"
#include "iowatch.h"

socketChan:: socketChan(ppsocket * _skt, ncp * _ncpController, IOWatch & _iow):
channel(_ncpController),
iow(_iow)
{
	skt = _skt;
	connectName = 0;
	iow.addIO(skt->socket());
	connected = false;
}

socketChan::~socketChan()
{
	iow.remIO(skt->socket());
	skt->closeSocket();
	delete skt;
	if (connectName)
		free(connectName);
}

void socketChan::
ncpDataCallback(bufferStore & a)
{
	if (connectName != 0) {
		skt->sendBufferStore(a);
	} else
		cerr << "socketchan: Connect without name!!!\n";
}

const char *socketChan::
getNcpConnectName()
{
	return connectName;
}

void socketChan::
ncpConnectAck()
{
	bufferStore a;
	a.addStringT("Ok");
	skt->sendBufferStore(a);
	connected = true;
}

void socketChan::
ncpConnectTerminate()
{
	bufferStore a;
	a.addStringT("Close");
	skt->sendBufferStore(a);
	terminateWhenAsked();
}

void socketChan::
socketPoll()
{
	if (connectName == 0) {
		bufferStore a;
		if (skt->getBufferStore(a, false) == 1) {
			connectName = strdup(a.getString());
			ncpConnect();
		}
	} else if (connected) {
		bufferStore a;
		int res = skt->getBufferStore(a, false);
		if (res == -1) {
			ncpDisconnect();
		} else if (res == 1) {
			if (a.getLen() > 5 &&
			    !strncmp(a.getString(), "Close", 5)) {
				ncpDisconnect();
			} else {
				ncpSend(a);
			}
		}
	}
}

bool socketChan::
isConnected()
const {
	return connected;
}
