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

#ifndef _ncp_h_
#define _ncp_h_

#include "bool.h"
#include "bufferstore.h"
class link;
class channel;
class IOWatch;

#define NCP_DEBUG_LOG  1
#define NCP_DEBUG_DUMP 2

class ncp {
	public:
		ncp(const char *fname, int baud, IOWatch &iow);
		~ncp();

		int connect(channel *c); // returns channel, or -1 if failure
		void disconnect(int channel);
		void send(int channel, bufferStore &a);
		void poll();
		void reset();
		bool stuffToSend();
		bool hasFailed();
		bool gotLinkChannel();
		void setVerbose(short int);
		short int getVerbose();
		void setLinkVerbose(short int);
		short int getLinkVerbose();
		void setPktVerbose(short int);
		short int getPktVerbose();
  
	private:
		enum c { MAX_LEN = 200, LAST_MESS = 1, NOT_LAST_MESS = 2 };
		enum interControllerMessageType {
			// Inter controller message types
			NCON_MSG_DATA_XOFF=1,
			NCON_MSG_DATA_XON=2,
			NCON_MSG_CONNECT_TO_SERVER=3,
			NCON_MSG_CONNECT_RESPONSE=4,
			NCON_MSG_CHANNEL_CLOSED=5,
			NCON_MSG_NCP_INFO=6,
			NCON_MSG_CHANNEL_DISCONNECT=7,
			NCON_MSG_NCP_END=8
		};
		int getFirstUnusedChan();
		void decodeControlMessage(bufferStore &buff);
		void controlChannel(int chan, enum interControllerMessageType t, bufferStore &command);
		char * ctrlMsgName(unsigned char);
  
		link *l;
		unsigned short verbose;
		channel *channelPtr[8];
		bufferStore messageList[8];
		int remoteChanList[8];
		bool gotLinkChan;
		bool failed;
};

#endif
