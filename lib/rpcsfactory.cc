//
//  RPCSFACTORY - factory object that creates an appropriate RPCS object
//  based on whatever the NCP daemon discovered in the INFO exchange.
//  Derived from rfsvfactory by Matt J. Gumbley <matt@gumbley.demon.co.uk>
//
//  Copyright (C) 2000 Fritz Elfert <felfert@to.com>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stream.h>
#include <stdlib.h>
#include <fstream.h>
#include <iomanip.h>
#include <time.h>
#include <string.h>

#include "bool.h"
#include "rpcs16.h"
#include "rpcs32.h"
#include "rpcsfactory.h"
#include "bufferstore.h"
#include "ppsocket.h"

rpcsfactory::rpcsfactory(ppsocket *_skt) //: serNum(0)
{
	skt = _skt;
}

rpcs * rpcsfactory::create(bool reconnect)
{
	// skt is connected to the ncp daemon, which will have (hopefully) seen
	// an INFO exchange, where the protocol version of the remote Psion was
	// sent, and noted. We have to ask the ncp daemon which protocol it saw,
	// so we can instantiate the correct rpcs protocol handler for the
	// caller. We announce ourselves to the NCP daemon, and the relevant
	// rpcs module will also announce itself.
	bufferStore a;
	a.init();
	a.addStringT("NCP$INFO");
	if (!skt->sendBufferStore(a)) {
		if (!reconnect)
			cerr << "rpcsfactory::create couldn't send version request" << endl;
		else {
			skt->closeSocket();
			// serNum = 0;
			skt->reconnect();
		}
		return NULL;
	}
	if (skt->getBufferStore(a) == 1) {
		if (a.getLen() > 8 && !strncmp(a.getString(), "Series 3", 8)) {
			return new rpcs16(skt);
		}
		else if (a.getLen() > 8 && !strncmp(a.getString(), "Series 5", 8)) {
			return new rpcs32(skt);
		}
		if ((a.getLen() > 8) && !strncmp(a.getString(), "No Psion", 8)) {
			skt->closeSocket();
			// serNum = 0;
			skt->reconnect();
			return NULL;
		}
		// Invalid protocol version
		cerr << "rpcsfactory::create received odd protocol version from
ncpd! (" << a << ")" << endl;
	} else {
		cerr << "rpcsfactory::create sent, response not 1" << endl;
	}

	// No message returned.
	return NULL;
}

