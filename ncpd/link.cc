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
#include <stdlib.h>
#include <stdio.h>

#include "defs.h"
#include "bool.h"
#include "link.h"
#include "packet.h"
#include "bufferstore.h"
#include "bufferarray.h"

link::link(const char *fname, int baud, IOWatch & iow, bool _s5, bool _verbose):
s5(_s5)
{
	p = new packet(fname, baud, iow, PACKET_LAYER_DIAGNOSTICS);
	verbose = _verbose;
	idSent = 0;
	idLastGot = -1;
	newLink = true;
	somethingToSend = false;
	timesSent = 0;
	failed = false;
}

link::~link()
{
	delete p;
}

void link::
send(const bufferStore & buff)
{
	if (buff.getLen() > 300)
		failed = true;
	else
		sendQueue.pushBuffer(buff);
}

bufferArray link::
poll()
{
	bufferArray ret;
	bufferStore buff;
	unsigned char type;

	while (p->get(type, buff)) {
		if ((type & 0xf0) == 0x30) {
			// Data
			int ser = type & 0x0f;
			if (verbose)
				cout << "link: Got data ser " << ser << " : " << buff << endl;
			// Send ack
			if (idLastGot != ser) {
				idLastGot = ser;
				ret.pushBuffer(buff);
			} else {
				if (verbose)
					cout << "link: Duplicated data - not passing back, repeating ack\n";
			}
			if (verbose)
				cout << "link: Send ack ser " << ser << endl;
			bufferStore blank;
			p->send(ser, blank);
			break;
		} else if ((type & 0xf0) == 0x00) {
			// Ack
			int ser = type & 0x0f;
			if (ser == idSent) {
				if (verbose)
					cout << "link: Got ack " << ser << " : " << buff << endl;
				somethingToSend = false;
				timesSent = 0;
			}
		} else if ((type & 0xf0) == 0x20) {
			// New link
			int ser = type & 0x0f;
			if (verbose)
				cout << "link: got New link request " << ser << " : " << buff << endl;
			idLastGot = 0;
			bufferStore blank;
			if (verbose)
				cout << "link: Sending ack of new link\n";
			somethingToSend = false;
			p->send(idLastGot, blank);
		} else if ((type & 0xf0) == 0x10) {
			// Disconnect
			cerr << "Disconnect?\n";
			failed = true;
			return ret;
		}
	}

	if (!somethingToSend) {
		countToResend = 0;
		if (newLink) {
			somethingToSend = true;
			toSend.init();
			newLink = false;
			idSent = 0;
		} else {
			if (!sendQueue.empty()) {
				somethingToSend = true;
				toSend = sendQueue.popBuffer();
				idSent++;
				if (idSent > 7)
					idSent = 0;
			}
		}
	}
	if (somethingToSend) {
		if (countToResend == 0) {
			timesSent++;
			if (timesSent == 5) {
				failed = true;
			} else {
				if (toSend.empty()) {
					// Request for new link
					if (verbose)
						cout << "link: Send req new session ser " << idSent << endl;
					p->send(0x20 + idSent, toSend);
				} else {
					if (verbose)
						cout << "link: Send data packet ser " << idSent << " : " << toSend << endl;
					p->send(0x30 + idSent, toSend);
				}
				countToResend = 5;
			}
		} else {
			countToResend--;
		}
	}
	return ret;
}

bool link::
stuffToSend()
{
	return (somethingToSend || !sendQueue.empty());
}

bool link::
hasFailed()
{
	return failed;
}
