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
#include <iomanip.h>

#include "linkchan.h"
#include "bufferstore.h"
#include "bufferarray.h"

linkChan::linkChan(ncp * _ncpController):channel(_ncpController)
{
	registerSer = 0x1234;
	ncpConnect();
}

void linkChan::
ncpDataCallback(bufferStore & a)
{
	int len = a.getLen();
	if (verbose & LINKCHAN_DEBUG_LOG) {
		cout << "linkchan: << msg ";
		if (verbose & LINKCHAN_DEBUG_DUMP)
			cout << a << endl;
		else
			cout << len << endl;
	}
	if ((len > 7) && (a.getByte(0) == 1)) {
		unsigned int ser = a.getWord(1);
		int res = a.getWord(3);
		// int dontknow = a.getWord(5);
		const char *srvName = a.getString(7);
		bufferArray newStack;
		bufferStore se;

		if (verbose & LINKCHAN_DEBUG_LOG)
			cout << "linkchan: received registerAck: ser=0x" << hex << setw(4)
				<< setfill(0) << ser << " res=" << res << " srvName=\""
				<< srvName << "\"" << endl;

		while (!registerStack.empty()) {
			se = registerStack.pop();
			if (se.getWord(0) == ser) {
				if (verbose & LINKCHAN_DEBUG_LOG)
					cout << "linkchan: found ser=0x" << hex << setw(4) <<
						setfill(0) << se.getWord(0) <<
						" on stack -> callBack to waiting chan" << endl;
				ncpDoRegisterAck((int)se.getWord(2));
			} else
				newStack += se;
		}
		registerStack = newStack;
		return;
	}
	cerr << "linkchan: unknown message " << a.getByte(0) << endl;
}

char *linkChan::
getNcpConnectName()
{
	return "LINK";
}

void linkChan::
ncpConnectAck()
{
	if (verbose & LINKCHAN_DEBUG_LOG)
		cout << "linkchan: << cack" << endl;
}

void linkChan::
ncpConnectTerminate()
{
	if (verbose & LINKCHAN_DEBUG_LOG)
		cout << "linkchan: << ctrm" << endl;
	terminateWhenAsked();
}

void linkChan::
ncpConnectNak()
{
	ncpConnectTerminate();
}

void linkChan::
Register(channel *ch)
{
	bufferStore a;
	bufferStore stack;

	stack.addWord(registerSer);
	stack.addWord(ch->getNcpChannel());
	registerStack += stack;
	a.addByte(0);
	a.addWord(registerSer++);
	a.addString(ch->getNcpConnectName());
	a.addByte(0);
	ncpSend(a);
}
