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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stream.h>
#include "stdio.h"
#include "string.h"
#include "malloc.h"

#include "socketchan.h"
#include "ncp.h"
#include "ppsocket.h"

socketChan:: socketChan(ppsocket * _skt, ncp * _ncpController):
    channel(_ncpController)
{
    skt = _skt;
    connectName = 0;
    connectTry = 0;
    connected = false;
}

socketChan::~socketChan()
{
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

char *socketChan::
getNcpConnectName()
{
    return connectName;
}

// NCP Command processing
bool socketChan::
ncpCommand(bufferStore & a)
{
    cerr << "socketChan:: received NCP command (" << a << ")" << endl;
    // str is guaranteed to begin with NCP$, and all NCP commands are
    // greater than or equal to 8 characters in length.
    const char *str = a.getString();
    // unsigned long len = a.getLen();
    bool ok = false;

    if (!strncmp(str+4, "INFO", 4)) {
	// Send information discovered during the receipt of the
	// NCON_MSG_NCP_INFO message.
	a.init();
	switch (ncpProtocolVersion()) {
	    case PV_SERIES_3:
		a.addStringT("Series 3");
		break;
	    case PV_SERIES_5:
		a.addStringT("Series 5");
		break;
	    default:
		cerr << "ncpd: protocol version not known" << endl;
		a.addStringT("Unknown!");
		break;
	}
	skt->sendBufferStore(a);
	ok = true;
    } else if (!strncmp(str+4, "CONN", 4)) {
	// Connect to a channel that was placed in 'pending' mode, by
	// checking the channel table against the ID...
	// DO ME LATER
	ok = true;
    } else if (!strncmp(str+4, "CHAL", 4)) {
	// Challenge
	// The idea is that the channel stays in 'secure' mode until a
	// valid RESP is received
	// DO ME LATER
	ok = true;
    } else if (!strncmp(str+4, "RESP", 4)) {
	// Reponse - here is the plaintext as sent in the CHAL - the
	// channel will only open up if this matches what ncpd has for
	// the encrypted plaintext.
	// DO ME LATER
	ok = true;
    }
    return ok;
}


void socketChan::
ncpConnectAck()
{
    bufferStore a;
    a.addStringT("Ok");
    skt->sendBufferStore(a);
    connected = true;
    connectTry = 3;
}

void socketChan::
ncpConnectTerminate()
{
    connectTry = 0;
    skt->closeSocket();
    terminateWhenAsked();
}

void socketChan::
ncpRegisterAck()
{
    connectTry++;
    ncpConnect();
}

void socketChan::
ncpConnectNak()
{
    if (!connectName || (connectTry > 1))
	ncpConnectTerminate();
    else {
	connectTry++;
	ncpRegister();
    }
}

void socketChan::
socketPoll()
{
    int res;

    if (connectName == 0) {
	bufferStore a;
	res = skt->getBufferStore(a, false);
	switch (res) {
	    case 1:
		// A client has connected, and is announcing who it
		// is...  e.g.  "SYS$RFSV.*"
		//
		// An NCP Channel can be in 'Control' or 'Data' mode.
		// Initially, it is in 'Control' mode, and can accept
		// certain commands.
		//
		// When a command is received that ncpd does not
		// understand, this is assumed to be a request to
		// connect to the remote service of that name, and enter
		// 'data' mode.
		//
		// Later, there might be an explicit command to enter
		// 'data' mode, and also a challenge-response protocol
		// before any connection can be made.
		//
		// All commands begin with "NCP$".
		
		// There is a magic process name called "NCP$INFO.*"
		// which is announced by the rfsvfactory. This causes a
		// response to be issued containing the NCP version
		// number. The rfsvfactory will create the correct type
		// of RFSV protocol handler, which will then announce
		// itself. So, first time in here, we might get the
		// NCP$INFO.*
		if (a.getLen() > 8 && !strncmp(a.getString(), "NCP$", 4)) {
		    if (!ncpCommand(a))
			cerr << "ncpd: command " << a << " unrecognized."
			     << endl;
		    return;
		}

		// This isn't a command, it's a remote process. Connect.
		connectName = strdup(a.getString());
		connectTry++;
		ncpConnect();
		break;
	    case -1:
		ncpConnectTerminate();
	       break;
	}
    } else if (connected) {
	bufferStore a;
	res = skt->getBufferStore(a, false);
	if (res == -1) {
	    ncpDisconnect();
	    skt->closeSocket();
	} else if (res == 1) {
	    ncpSend(a);
	}
    }
}

bool socketChan::
isConnected()
const {
    return connected;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
