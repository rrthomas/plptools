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

#include "rpcs.h"
#include "bufferstore.h"
#include "ppsocket.h"
#include "bufferarray.h"
#include "Enum.h"

ENUM_DEFINITION(rpcs::machs, rpcs::PSI_MACH_UNKNOWN) {
	stringRep.add(rpcs::PSI_MACH_UNKNOWN,	"Unknown device");
	stringRep.add(rpcs::PSI_MACH_PC,	"PC");
	stringRep.add(rpcs::PSI_MACH_MC,	"MC");
	stringRep.add(rpcs::PSI_MACH_HC,	"HC");
	stringRep.add(rpcs::PSI_MACH_S3,	"Series 3");
	stringRep.add(rpcs::PSI_MACH_S3A,	"Series 3a, 3c or 3mx");
	stringRep.add(rpcs::PSI_MACH_WORKABOUT,	"Workabout");
	stringRep.add(rpcs::PSI_MACH_SIENNA,	"Sienna");
	stringRep.add(rpcs::PSI_MACH_S3C,	"Series 3c");
	stringRep.add(rpcs::PSI_MACH_S5,	"Series 5");
	stringRep.add(rpcs::PSI_MACH_WINC,	"WinC");
}

ENUM_DEFINITION(rpcs::batterystates, rpcs::PSI_BATT_DEAD) {
	stringRep.add(rpcs::PSI_BATT_DEAD,	"Empty");
	stringRep.add(rpcs::PSI_BATT_VERYLOW,	"Very Low");
	stringRep.add(rpcs::PSI_BATT_LOW,	"Low");
	stringRep.add(rpcs::PSI_BATT_GOOD,	"Good");
}

ENUM_DEFINITION(rpcs::languages, rpcs::PSI_LANG_TEST) {
	stringRep.add(rpcs::PSI_LANG_TEST,	"Test");
	stringRep.add(rpcs::PSI_LANG_en_GB,	"English");
	stringRep.add(rpcs::PSI_LANG_de_DE,	"German");
	stringRep.add(rpcs::PSI_LANG_fr_FR,	"French");
	stringRep.add(rpcs::PSI_LANG_es_ES,	"Spanish");
	stringRep.add(rpcs::PSI_LANG_it_IT,	"Italian");
	stringRep.add(rpcs::PSI_LANG_sv_SE,	"Swedish");
	stringRep.add(rpcs::PSI_LANG_da_DK,	"Danish");
	stringRep.add(rpcs::PSI_LANG_no_NO,	"Norwegian");
	stringRep.add(rpcs::PSI_LANG_fi_FI,	"Finnish");
	stringRep.add(rpcs::PSI_LANG_en_US,	"American");
	stringRep.add(rpcs::PSI_LANG_fr_CH,	"Swiss French");
	stringRep.add(rpcs::PSI_LANG_de_CH,	"Swiss German");
	stringRep.add(rpcs::PSI_LANG_pt_PT,	"Portugese");
	stringRep.add(rpcs::PSI_LANG_tr_TR,	"Turkish");
	stringRep.add(rpcs::PSI_LANG_is_IS,	"Icelandic");
	stringRep.add(rpcs::PSI_LANG_ru_RU,	"Russian");
	stringRep.add(rpcs::PSI_LANG_hu_HU,	"Hungarian");
	stringRep.add(rpcs::PSI_LANG_nl_NL,	"Dutch");
	stringRep.add(rpcs::PSI_LANG_nl_BE,	"Belgian Flemish");
	stringRep.add(rpcs::PSI_LANG_en_AU,	"Australian");
	stringRep.add(rpcs::PSI_LANG_fr_BE,	"Belgish French");
	stringRep.add(rpcs::PSI_LANG_de_AT,	"Austrian");
	stringRep.add(rpcs::PSI_LANG_en_NZ,	"New Zealand"); // FIXME: not shure about ISO code
	stringRep.add(rpcs::PSI_LANG_fr_CA,	"International French"); // FIXME: not shure about ISO code
	stringRep.add(rpcs::PSI_LANG_cs_CZ,	"Czech");
	stringRep.add(rpcs::PSI_LANG_sk_SK,	"Slovak");
	stringRep.add(rpcs::PSI_LANG_pl_PL,	"Polish");
	stringRep.add(rpcs::PSI_LANG_sl_SI,	"Slovenian");
}

//
// public common API
//
void rpcs::
reconnect(void)
{
	skt->closeSocket();
	skt->reconnect();
	reset();
}

void rpcs::
reset(void)
{
	bufferStore a;
	status = rfsv::E_PSI_FILE_DISC;
	a.addStringT(getConnectName());
	if (skt->sendBufferStore(a)) {
		if (skt->getBufferStore(a) == 1) {
			if (!strcmp(a.getString(0), "Ok"))
				status = rfsv::E_PSI_GEN_NONE;
		}
	}
}

Enum<rfsv::errs> rpcs::
getStatus(void)
{
	return status;
}

const char *rpcs::
getConnectName(void)
{
	return "SYS$RPCS";
}

//
// protected internals
//
bool rpcs::
sendCommand(enum commands cc, bufferStore & data)
{
	if (status == rfsv::E_PSI_FILE_DISC) {
		reconnect();
		if (status == rfsv::E_PSI_FILE_DISC)
			return false;
	}
	bool result;
	bufferStore a;
	a.addByte(cc);
	a.addBuff(data);
	result = skt->sendBufferStore(a);
	if (!result) {
		reconnect();
		result = skt->sendBufferStore(a);
		if (!result)
			status = rfsv::E_PSI_FILE_DISC;
	}
	return result;
}

Enum<rfsv::errs> rpcs::
getResponse(bufferStore & data, bool statusIsFirstByte)
{
	Enum<rfsv::errs> ret;
	if (skt->getBufferStore(data) == 1) {
		if (statusIsFirstByte) {
			ret = (enum rfsv::errs)((char)data.getByte(0));
			data.discardFirstBytes(1);
		} else {
			int l = data.getLen();
			if (l > 0) {
				ret = (enum rfsv::errs)((char)data.getByte(data.getLen() - 1));
				data.init((const unsigned char *)data.getString(), l - 1);
			} else
				ret = rfsv::E_PSI_GEN_FAIL;
		}
		return ret;
	} else
		status = rfsv::E_PSI_FILE_DISC;
	return status;
}

//
// APIs, identical on SIBO and EPOC
//
Enum<rfsv::errs> rpcs::
getNCPversion(int &major, int &minor)
{
	Enum<rfsv::errs> res;
	bufferStore a;

	if (!sendCommand(QUERY_NCP, a))
		return rfsv::E_PSI_FILE_DISC;
	if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
		return res;
	if (a.getLen() != 2)
		return rfsv::E_PSI_GEN_FAIL;
	major = a.getByte(0);
	minor = a.getByte(1);
	return res;
}

Enum<rfsv::errs> rpcs::
execProgram(const char *program, const char *args)
{
	bufferStore a;

	a.addStringT(program);
	int l = strlen(program);
	for (int i = 127; i > l; i--)
		a.addByte(0);

	/**
	 * This is a hack for the jotter app on mx5 pro. (and probably others)
	 * Jotter seems to read it's arguments one char past normal apps.
	 * Without this hack, The Drive-Character gets lost. Other apps don't
	 * seem to be hurt by the additional blank.
	 */
	a.addByte(strlen(args) + 1);
	a.addByte(' ');

	a.addStringT(args);
	if (!sendCommand(EXEC_PROG, a))
		return rfsv::E_PSI_FILE_DISC;
	return getResponse(a, true);
}

Enum<rfsv::errs> rpcs::
stopProgram(const char *program)
{
	bufferStore a;

	a.addStringT(program);
	if (!sendCommand(STOP_PROG, a))
		return rfsv::E_PSI_FILE_DISC;
	return getResponse(a, true);
}

Enum<rfsv::errs> rpcs::
queryProgram(const char *program)
{
	bufferStore a;

	a.addStringT(program);
	if (!sendCommand(QUERY_PROG, a))
		return rfsv::E_PSI_FILE_DISC;
	return getResponse(a, true);
}

Enum<rfsv::errs> rpcs::
formatOpen(const char *drive, int &handle, int &count)
{
	Enum<rfsv::errs> res;
	bufferStore a;

	a.addStringT(drive);
	if (!sendCommand(FORMAT_OPEN, a))
		return rfsv::E_PSI_FILE_DISC;
	if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
		return res;
	if (a.getLen() != 4)
		return rfsv::E_PSI_GEN_FAIL;
	handle = a.getWord(0);
	count = a.getWord(2);
	return res;
}

Enum<rfsv::errs> rpcs::
formatRead(int handle)
{
	bufferStore a;

	a.addWord(handle);
	if (!sendCommand(FORMAT_READ, a))
		return rfsv::E_PSI_FILE_DISC;
	return getResponse(a, true);
}

Enum<rfsv::errs> rpcs::
getUniqueID(const char *device, long &id)
{
	Enum<rfsv::errs> res;
	bufferStore a;

	a.addStringT(device);
	if (!sendCommand(GET_UNIQUEID, a))
		return rfsv::E_PSI_FILE_DISC;
	if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
		return res;
	if (a.getLen() != 4)
		return rfsv::E_PSI_GEN_FAIL;
	id = a.getDWord(0);
	return res;
}

Enum<rfsv::errs> rpcs::
getOwnerInfo(bufferArray &owner)
{
	Enum<rfsv::errs> res;
	bufferStore a;

	if (!sendCommand(GET_OWNERINFO, a))
		return rfsv::E_PSI_FILE_DISC;
	if ((res = (enum rfsv::errs)getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
		return res;
	a.addByte(0);
	int l = a.getLen();
	char *s = (char *)a.getString(0);
	for (int i = 0; i < l; i++)
		if (s[i] == 6)
			s[i] = 0;
	owner.clear();
	while (l > 0) {
		if (*s != '\0') {
			bufferStore b;
			b.addStringT(s);
			owner += b;
			l -= (strlen(s) + 1);
			s += (strlen(s) + 1);
		} else {
			l--;
			s++;
		}
	}
	return res;
}

Enum<rfsv::errs> rpcs::
getMachineType(Enum<machs> &type)
{
	Enum<rfsv::errs> res;
	bufferStore a;

	if (!sendCommand(GET_MACHINETYPE, a))
		return rfsv::E_PSI_FILE_DISC;
	if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
		return res;
	if (a.getLen() != 2)
		return rfsv::E_PSI_GEN_FAIL;
	type = (enum machs)a.getWord(0);
	return res;
}

Enum<rfsv::errs> rpcs::
fuser(const char *name, char *buf, int maxlen)
{
	Enum<rfsv::errs> res;
	bufferStore a;
	char *p;

	a.addStringT(name);
	if (!sendCommand(FUSER, a))
		return rfsv::E_PSI_FILE_DISC;
	if ((res = getResponse(a, true)) != rfsv::E_PSI_GEN_NONE)
		return res;
	strncpy(buf, a.getString(0), maxlen - 1);
	while ((p = strchr(buf, 6)))
		*p = '\0';
	return res;
}

Enum<rfsv::errs> rpcs::
quitServer(void)
{
	bufferStore a;
	if (!sendCommand(QUIT_SERVER, a))
		return rfsv::E_PSI_FILE_DISC;
	return getResponse(a, true);
}
