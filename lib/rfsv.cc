#include "rfsv.h"
#include "ppsocket.h"
#include "bufferstore.h"
#include "Enum.h"

ENUM_DEFINITION(rfsv::errs, rfsv::E_PSI_GEN_NONE) {
	stringRep.add(rfsv::E_PSI_GEN_NONE,		"no error");
	stringRep.add(rfsv::E_PSI_GEN_FAIL,		"general");
	stringRep.add(rfsv::E_PSI_GEN_ARG,		"bad argument");
	stringRep.add(rfsv::E_PSI_GEN_OS,		"OS error");
	stringRep.add(rfsv::E_PSI_GEN_NSUP,		"not supported");
	stringRep.add(rfsv::E_PSI_GEN_UNDER,		"numeric underflow");
	stringRep.add(rfsv::E_PSI_GEN_OVER,		"numeric overflow");
	stringRep.add(rfsv::E_PSI_GEN_RANGE,		"numeric exception");
	stringRep.add(rfsv::E_PSI_GEN_INUSE,		"in use");
	stringRep.add(rfsv::E_PSI_GEN_NOMEMORY,		"out of memory");
	stringRep.add(rfsv::E_PSI_GEN_NOSEGMENTS,	"out of segments");
	stringRep.add(rfsv::E_PSI_GEN_NOSEM,		"out of semaphores");
	stringRep.add(rfsv::E_PSI_GEN_NOPROC,		"out of processes");
	stringRep.add(rfsv::E_PSI_GEN_OPEN,		"already open");
	stringRep.add(rfsv::E_PSI_GEN_NOTOPEN,		"not open");
	stringRep.add(rfsv::E_PSI_GEN_IMAGE,		"bad image");
	stringRep.add(rfsv::E_PSI_GEN_RECEIVER,		"receiver error");
	stringRep.add(rfsv::E_PSI_GEN_DEVICE,		"device error");
	stringRep.add(rfsv::E_PSI_GEN_FSYS,		"no filesystem");
	stringRep.add(rfsv::E_PSI_GEN_START,		"not ready");
	stringRep.add(rfsv::E_PSI_GEN_NOFONT,		"no font");
	stringRep.add(rfsv::E_PSI_GEN_TOOWIDE,		"too wide");
	stringRep.add(rfsv::E_PSI_GEN_TOOMANY,		"too many");
	stringRep.add(rfsv::E_PSI_FILE_EXIST,		"file already exists");
	stringRep.add(rfsv::E_PSI_FILE_NXIST,		"no such file");
	stringRep.add(rfsv::E_PSI_FILE_WRITE,		"write error");
	stringRep.add(rfsv::E_PSI_FILE_READ,		"read error");
	stringRep.add(rfsv::E_PSI_FILE_EOF,		"end of file");
	stringRep.add(rfsv::E_PSI_FILE_FULL,		"disk/serial read buffer full");
	stringRep.add(rfsv::E_PSI_FILE_NAME,		"invalid name");
	stringRep.add(rfsv::E_PSI_FILE_ACCESS,		"access denied");
	stringRep.add(rfsv::E_PSI_FILE_LOCKED,		"ressource locked");
	stringRep.add(rfsv::E_PSI_FILE_DEVICE,		"no such device");
	stringRep.add(rfsv::E_PSI_FILE_DIR,		"no such directory");
	stringRep.add(rfsv::E_PSI_FILE_RECORD,		"no such record");
	stringRep.add(rfsv::E_PSI_FILE_RDONLY,		"file is read-only");
	stringRep.add(rfsv::E_PSI_FILE_INV,		"invalid I/O operation");
	stringRep.add(rfsv::E_PSI_FILE_PENDING,		"I/O pending (not yet completed)");
	stringRep.add(rfsv::E_PSI_FILE_VOLUME,		"invalid volume name");
	stringRep.add(rfsv::E_PSI_FILE_CANCEL,		"cancelled");
	stringRep.add(rfsv::E_PSI_FILE_ALLOC,		"no memory for control block");
	stringRep.add(rfsv::E_PSI_FILE_DISC,		"unit disconnected");
	stringRep.add(rfsv::E_PSI_FILE_CONNECT,		"already connected");
	stringRep.add(rfsv::E_PSI_FILE_RETRAN,		"retransmission threshold exceeded");
	stringRep.add(rfsv::E_PSI_FILE_LINE,		"physical link failure");
	stringRep.add(rfsv::E_PSI_FILE_INACT,		"inactivity timer expired");
	stringRep.add(rfsv::E_PSI_FILE_PARITY,		"serial parity error");
	stringRep.add(rfsv::E_PSI_FILE_FRAME,		"serial framing error");
	stringRep.add(rfsv::E_PSI_FILE_OVERRUN,		"serial overrun error");
	stringRep.add(rfsv::E_PSI_MDM_CONFAIL,		"modem cannot connect to remote modem");
	stringRep.add(rfsv::E_PSI_MDM_BUSY,		"remote modem busy");
	stringRep.add(rfsv::E_PSI_MDM_NOANS,		"remote modem did not answer");
	stringRep.add(rfsv::E_PSI_MDM_BLACKLIST,	"number blacklisted by the modem");
	stringRep.add(rfsv::E_PSI_FILE_NOTREADY,	"drive not ready");
	stringRep.add(rfsv::E_PSI_FILE_UNKNOWN,		"unknown media");
	stringRep.add(rfsv::E_PSI_FILE_DIRFULL,		"directory full");
	stringRep.add(rfsv::E_PSI_FILE_PROTECT,		"write-protected");
	stringRep.add(rfsv::E_PSI_FILE_CORRUPT,		"media corrupt");
	stringRep.add(rfsv::E_PSI_FILE_ABORT,		"aborted operation");
	stringRep.add(rfsv::E_PSI_FILE_ERASE,		"failed to erase flash media");
	stringRep.add(rfsv::E_PSI_FILE_INVALID,		"invalid file for DBF system");
	stringRep.add(rfsv::E_PSI_GEN_POWER,		"power failure");
	stringRep.add(rfsv::E_PSI_FILE_TOOBIG,		"too big");
	stringRep.add(rfsv::E_PSI_GEN_DESCR,		"bad descriptor");
	stringRep.add(rfsv::E_PSI_GEN_LIB,		"bad entry point");
	stringRep.add(rfsv::E_PSI_FILE_NDISC,		"could not diconnect");
	stringRep.add(rfsv::E_PSI_FILE_DRIVER,		"bad driver");
	stringRep.add(rfsv::E_PSI_FILE_COMPLETION,	"operation not completed");
	stringRep.add(rfsv::E_PSI_GEN_BUSY,		"server busy");
	stringRep.add(rfsv::E_PSI_GEN_TERMINATED,	"terminated");
	stringRep.add(rfsv::E_PSI_GEN_DIED,		"died");
	stringRep.add(rfsv::E_PSI_FILE_HANDLE,		"bad handle");
	stringRep.add(rfsv::E_PSI_NOT_SIBO,		"invalid operation for RFSV16");
	stringRep.add(rfsv::E_PSI_INTERNAL,		"libplp internal error");
}

const char *rfsv::getConnectName(void) {
	return "SYS$RFSV";
}

rfsv::~rfsv() {
	bufferStore a;
	a.addStringT("Close");
	if (status == E_PSI_GEN_NONE)
		skt->sendBufferStore(a);
	skt->closeSocket();
}

void rfsv::reconnect(void)
{
	skt->closeSocket();
	skt->reconnect();
	serNum = 0;
	reset();
}

void rfsv::reset(void) {
	bufferStore a;
	status = E_PSI_FILE_DISC;
	a.addStringT(getConnectName());
	if (skt->sendBufferStore(a)) {
		if (skt->getBufferStore(a) == 1) {
			if (!strcmp(a.getString(0), "Ok"))
				status = E_PSI_GEN_NONE;
		}
	}
}

Enum<rfsv::errs> rfsv::getStatus(void) {
	return status;
}

string rfsv::
convertSlash(const string &name)
{
	string tmp = "";
	for (const char *p = name.c_str(); *p; p++)
		tmp += (*p == '/') ? '\\' : *p;
	return tmp;
}

string rfsv::
attr2String(const u_int32_t attr)
{
	string tmp = "";
	tmp += ((attr & PSI_A_DIR) ? 'd' : '-');
	tmp += ((attr & PSI_A_READ) ? 'r' : '-');
	tmp += ((attr & PSI_A_RDONLY) ? '-' : 'w');
	tmp += ((attr & PSI_A_HIDDEN) ? 'h' : '-');
	tmp += ((attr & PSI_A_SYSTEM) ? 's' : '-');
	tmp += ((attr & PSI_A_ARCHIVE) ? 'a' : '-');
	tmp += ((attr & PSI_A_VOLUME) ? 'v' : '-');

	// EPOC
	tmp += ((attr & PSI_A_NORMAL) ? 'n' : '-');
	tmp += ((attr & PSI_A_TEMP) ? 't' : '-');
	tmp += ((attr & PSI_A_COMPRESSED) ? 'c' : '-');
	// SIBO
	tmp[7] = ((attr & PSI_A_EXEC) ? 'x' : tmp[7]);
	tmp[8] = ((attr & PSI_A_STREAM) ? 'b' : tmp[8]);
	tmp[9] = ((attr & PSI_A_TEXT) ? 't' : tmp[9]);
	return tmp;
}

