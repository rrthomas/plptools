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

link::link(const char *fname, int baud, IOWatch & iow, unsigned short _verbose)
{
	p = new packet(fname, baud, iow);
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
reset() {
	idSent = 0;
	idLastGot = -1;
	newLink = true;
	somethingToSend = false;
	timesSent = 0;
	failed = false;
}

short int link::
getVerbose()
{
	return verbose;
}

void link::
setVerbose(short int _verbose)
{
	verbose = _verbose;
}

short int link::
getPktVerbose()
{
	return p->getVerbose();
}

void link::
setPktVerbose(short int _verbose)
{
	p->setVerbose(_verbose);
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
		int seq = type & 0x0f;
		bufferStore blank;
		type &= 0xf0;
		switch (type) {
			case 0x30:
				if (verbose & LNK_DEBUG_LOG) {
					cout << "link: << dat seq=" << seq ;
					if (verbose & LNK_DEBUG_DUMP)
						cout << " " << buff << endl;
					else
						cout << " len=" << buff.getLen() << endl;
				}
				// Send ack
				if (idLastGot != seq) {
					idLastGot = seq;
					ret.pushBuffer(buff);
				} else {
					if (verbose & LNK_DEBUG_LOG)
						cout << "link: DUP\n";
				}
				if (verbose & LNK_DEBUG_LOG)
					cout << "link: >> ack seq=" << seq << endl;
				blank.init();
				p->send(seq, blank);
				break;
			case 0x00:
				// Ack
				if (seq == idSent) {
					if (verbose & LNK_DEBUG_LOG) {
						cout << "link: << ack seq=" << seq ;
						if (verbose & LNK_DEBUG_DUMP)
							cout << " " << buff;
						cout << endl;
					}
					somethingToSend = false;
					timesSent = 0;
				}
				break;
			case 0x20:
				// New link
				if (verbose & LNK_DEBUG_LOG) {
					cout << "link: << lrq seq=" << seq;
					if (verbose & LNK_DEBUG_DUMP)
						cout << " " << buff;
					cout << endl;
				}
				idLastGot = 0;
				if (verbose & LNK_DEBUG_LOG)
					cout << "link: >> lack seq=" << seq << endl;
				somethingToSend = false;
				blank.init();
				p->send(idLastGot, blank);
				break;
			case 0x10:
				// Disconnect
				if (verbose & LNK_DEBUG_LOG)
					cout << "link: << DISC" << endl;
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
					if (verbose & LNK_DEBUG_LOG)
						cout << "link: >> lrq seq=" << idSent << endl;
					p->send(0x20 + idSent, toSend);
				} else {
					if (verbose & LNK_DEBUG_LOG) {
						cout << "link: >> data seq=" << idSent;
						if (verbose & LNK_DEBUG_DUMP)
							cout << " " << toSend;
						cout << endl;
					}
					p->send(0x30 + idSent, toSend);
				}
				countToResend = 5;
			}
		} else
			countToResend--;
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
