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

#ifndef _link_h_
#define _link_h_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include "bufferstore.h"
#include "bufferarray.h"

#define LNK_DEBUG_LOG  1
#define LNK_DEBUG_DUMP 2

class packet;
class IOWatch;

class link {
	public:
  		link(const char *fname, int baud, IOWatch &iow, unsigned short _verbose = 0);
  		~link();
  		void send(const bufferStore &buff);
  		bufferArray poll();
  		bool stuffToSend();
  		bool hasFailed();
		void reset();
		void flush();
		void setVerbose(short int);
		short int getVerbose();
		void setPktVerbose(short int);
		short int getPktVerbose();
  
	private:
  		packet *p;
  		int idSent;
  		int countToResend;
  		int timesSent;
  		bufferArray sendQueue;
  		bufferStore toSend;
  		int idLastGot;
  		bool newLink;
  		unsigned short verbose;
  		bool somethingToSend;
  		bool failed;
};

#endif
