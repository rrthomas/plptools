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

#ifndef _linkchan_h_
#define _linkchan_h_

#include "channel.h"

#define LINKCHAN_DEBUG_LOG  1
#define LINKCHAN_DEBUG_DUMP 2

class linkChan : public channel {
	public:
		linkChan(ncp *ncpController);
  
		void ncpDataCallback(bufferStore &a);
		const char *getNcpConnectName();
		void ncpConnectAck();
		void ncpConnectTerminate();
};

#endif
