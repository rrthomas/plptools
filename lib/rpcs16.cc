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
#include "bufferstore.h"
#include "ppsocket.h"

rpcs16::rpcs16(ppsocket * _skt)
{
	skt = _skt;
	reset();
}

rpcs16::~rpcs16()
{
	bufferStore a;
	a.addStringT("Close");
	if (status == rfsv::E_PSI_GEN_NONE)
		skt->sendBufferStore(a);
	skt->closeSocket();
}

Enum<rfsv::errs> rpcs16::
queryDrive(char drive, bufferArray &ret)
{
	bufferStore a;
	a.addByte(drive);
	if (!sendCommand(rpcs::QUERY_DRIVE, a))
		return rfsv::E_PSI_FILE_DISC;
	Enum<rfsv::errs> res = getResponse(a);
cout << dec << "qd: " << res << " " << a.getLen() << " a="<< a << endl;
	return res;
}

Enum<rfsv::errs> rpcs16::
getCmdLine(const char *process, bufferStore &ret)
{
	return rfsv::E_PSI_GEN_NONE;
}
