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

#ifndef _channel_h_
#define _channel_h_

#include <stdio.h>

#include "bool.h"

class ncp;
class bufferStore;

class channel {
	public:
		channel(ncp *ncpController);
		virtual ~channel() {}
		void newNcpController(ncp *ncpController);

		void setNcpChannel(int chan);
		int getNcpChannel(void);
		void ncpSend(bufferStore &a);
		void setVerbose(short int _verbose);
		short int getVerbose();
		virtual void ncpDataCallback(bufferStore &a) = NULL;
		virtual char *getNcpConnectName() = NULL;
		void ncpConnect();
		void ncpRegister();
		void ncpDoRegisterAck(int);
		virtual void ncpConnectAck() = NULL;
		virtual void ncpConnectTerminate() = NULL;
		virtual void ncpConnectNak() = NULL;
		virtual void ncpRegisterAck() = NULL;
		void ncpDisconnect();
		short int ncpProtocolVersion();

		// The following two calls are used for destructing an instance
		virtual bool terminate(); // Mainloop will terminate this class if true
		void terminateWhenAsked();

	protected:
		short int verbose;

	private:
		ncp *ncpController;
		int ncpChannel;
		bool _terminate;
};

#endif
