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

#ifndef _socketchan_h_
#define _socketchan_h_

#include "bool.h"
#include "channel.h"
class ppsocket;
class IOWatch;

class socketChan : public channel {
public:
  socketChan(ppsocket* comms, ncp* ncpController, IOWatch &iow);
  virtual ~socketChan();

  void ncpDataCallback(bufferStore& a);
  char* getNcpConnectName();
  void ncpConnectAck();
  void ncpRegisterAck();
  void ncpDoRegisterAck(int) {}
  void ncpConnectTerminate();
  void ncpConnectNak();

  bool isConnected() const;
  void socketPoll();
private:
  enum protocolVersionType { PV_SERIES_5 = 6, PV_SERIES_3 = 3 };
  bool ncpCommand(bufferStore &a);
  ppsocket* skt;
  IOWatch &iow;
  char* connectName;
  bool connected;
  int connectTry;
};

#endif
