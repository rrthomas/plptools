#include "rfsv.h"

char *rfsv::
opErr(long status)
{
	enum errs e = (enum errs) status;
	switch (e) {
		case E_PSI_GEN_NONE:
			return "no error";
		case E_PSI_GEN_FAIL:
			return "general";
		case E_PSI_GEN_ARG:
			return "bad argument";
		case E_PSI_GEN_OS:
			return "OS error";
		case E_PSI_GEN_NSUP:
			return "not supported";
		case E_PSI_GEN_UNDER:
			return "numeric underflow";
		case E_PSI_GEN_OVER:
			return "numeric overflow";
		case E_PSI_GEN_RANGE:
			return "numeric exception";
		case E_PSI_GEN_INUSE:
			return "in use";
		case E_PSI_GEN_NOMEMORY:
			return "out of memory";
		case E_PSI_GEN_NOSEGMENTS:
			return "out of segments";
		case E_PSI_GEN_NOSEM:
			return "out of semaphores";
		case E_PSI_GEN_NOPROC:
			return "out of processes";
		case E_PSI_GEN_OPEN:
			return "already open";
		case E_PSI_GEN_NOTOPEN:
			return "not open";
		case E_PSI_GEN_IMAGE:
			return "bad image";
		case E_PSI_GEN_RECEIVER:
			return "receiver error";
		case E_PSI_GEN_DEVICE:
			return "device error";
		case E_PSI_GEN_FSYS:
			return "no filesystem";
		case E_PSI_GEN_START:
			return "not ready";
		case E_PSI_GEN_NOFONT:
			return "no font";
		case E_PSI_GEN_TOOWIDE:
			return "too wide";
		case E_PSI_GEN_TOOMANY:
			return "too many";
		case E_PSI_FILE_EXIST:
			return "file already exists";
		case E_PSI_FILE_NXIST:
			return "no such file";
		case E_PSI_FILE_WRITE:
			return "write error";
		case E_PSI_FILE_READ:
			return "read error";
		case E_PSI_FILE_EOF:
			return "end of file";
		case E_PSI_FILE_FULL:
			return "disk/serial read buffer full";
		case E_PSI_FILE_NAME:
			return "invalid name";
		case E_PSI_FILE_ACCESS:
			return "access denied";
		case E_PSI_FILE_LOCKED:
			return "ressource locked";
		case E_PSI_FILE_DEVICE:
			return "no such device";
		case E_PSI_FILE_DIR:
			return "no such directory";
		case E_PSI_FILE_RECORD:
			return "no such record";
		case E_PSI_FILE_RDONLY:
			return "file is read-only";
		case E_PSI_FILE_INV:
			return "invalid I/O operation";
		case E_PSI_FILE_PENDING:
			return "I/O pending (not yet completed)";
		case E_PSI_FILE_VOLUME:
			return "invalid volume name";
		case E_PSI_FILE_CANCEL:
			return "cancelled";
		case E_PSI_FILE_ALLOC:
			return "no memory for control block";
		case E_PSI_FILE_DISC:
			return "unit disconnected";
		case E_PSI_FILE_CONNECT:
			return "already connected";
		case E_PSI_FILE_RETRAN:
			return "retransmission threshold exceeded";
		case E_PSI_FILE_LINE:
			return "physical link failure";
		case E_PSI_FILE_INACT:
			return "inactivity timer expired";
		case E_PSI_FILE_PARITY:
			return "serial parity error";
		case E_PSI_FILE_FRAME:
			return "serial framing error";
		case E_PSI_FILE_OVERRUN:
			return "serial overrun error";
		case E_PSI_MDM_CONFAIL:
			return "modem cannot connect to remote modem";
		case E_PSI_MDM_BUSY:
			return "remote modem busy";
		case E_PSI_MDM_NOANS:
			return "remote modem did not answer";
		case E_PSI_MDM_BLACKLIST:
			return "number blacklisted by the modem";
		case E_PSI_FILE_NOTREADY:
			return "drive not ready";
		case E_PSI_FILE_UNKNOWN:
			return "unknown media";
		case E_PSI_FILE_DIRFULL:
			return "directory full";
		case E_PSI_FILE_PROTECT:
			return "write-protected";
		case E_PSI_FILE_CORRUPT:
			return "media corrupt";
		case E_PSI_FILE_ABORT:
			return "aborted operation";
		case E_PSI_FILE_ERASE:
			return "failed to erase flash media";
		case E_PSI_FILE_INVALID:
			return "invalid file for DBF system";
		case E_PSI_GEN_POWER:
			return "power failure";
		case E_PSI_FILE_TOOBIG:
			return "too big";
		case E_PSI_GEN_DESCR:
			return "bad descriptor";
		case E_PSI_GEN_LIB:
			return "bad entry point";
		case E_PSI_FILE_NDISC:
			return "could not diconnect";
		case E_PSI_FILE_DRIVER:
			return "bad driver";
		case E_PSI_FILE_COMPLETION:
			return "operation not completed";
		case E_PSI_GEN_BUSY:
			return "server busy";
		case E_PSI_GEN_TERMINATED:
			return "terminated";
		case E_PSI_GEN_DIED:
			return "died";
		case E_PSI_FILE_HANDLE:
			return "bad handle";
		case E_PSI_NOT_SIBO:
			return "invalid operation for RFSV16";
		default:
			return "Unknown error";
	}
}

