#ifndef _rfsv32_h_
#define _rfsv32_h_

#include "rfsv.h"

class ppsocket;
class bufferStore;
class bufferArray;

class rfsv32 : public rfsv {
	public:
	rfsv32(ppsocket *);
	~rfsv32();
	void reset();
	void reconnect();

	long dir(const char *, bufferArray *);
	long dircount(const char *, long *);
	long copyFromPsion(const char *, const char *, cpCallback_t);
	long copyToPsion(const char *, const char *, cpCallback_t);
	long mkdir(const char *);
	long rmdir(const char *);
	long remove(const char *);
	long rename(const char *, const char *);
	long mktemp(long *, char *);
	long fgeteattr(const char *, long *, long *, long *);
	long fgetattr(const char *, long *);
	long fsetattr(const char *, long, long);
	long fgetmtime(const char *, long *);
	long fsetmtime(const char *, long);
	long fopendir(long, const char *, long &);
	long fopen(long, const char *, long &);
	long fcreatefile(long, const char *, long &);
	long freplacefile(long, const char *, long &);
	long fseek(long, long, long);
	long fread(long, unsigned char *, long);
	long fwrite(long, unsigned char *, long);
	long fsetsize(long, long);
	long fclose(long);

	long devlist(long *);
	char *devinfo(int, long *, long *, long *, long *);
	long getStatus();
	char *opAttr(long);
	long opMode(long);

	long attr2std(long);
	long std2attr(long);

	private:
	
	enum file_attrib {
		EPOC_ATTR_RONLY      = 0x0001,
		EPOC_ATTR_HIDDEN     = 0x0002,
		EPOC_ATTR_SYSTEM     = 0x0004,
		EPOC_ATTR_DIRECTORY  = 0x0010,
		EPOC_ATTR_ARCHIVE    = 0x0020,
		EPOC_ATTR_VOLUME     = 0x0040,
		EPOC_ATTR_NORMAL     = 0x0080,
		EPOC_ATTR_TEMPORARY  = 0x0100,
		EPOC_ATTR_COMPRESSED = 0x0800
	};

	enum open_mode {
		EPOC_OMODE_SHARE_EXCLUSIVE = 0x0000,
		EPOC_OMODE_SHARE_READERS = 0x0001,
		EPOC_OMODE_SHARE_ANY = 0x0002,
		EPOC_OMODE_BINARY = 0x0000,
		EPOC_OMODE_TEXT = 0x0020,
		EPOC_OMODE_READ_WRITE = 0x0200
	};

	enum epoc_errs {
		E_EPOC_NONE = 0,
		E_EPOC_NOT_FOUND = -1,
		E_EPOC_GENERAL = -2,
		E_EPOC_CANCEL = -3,
		E_EPOC_NO_MEMORY = -4,
		E_EPOC_NOT_SUPPORTED = -5,
		E_EPOC_ARGUMENT = -6,
		E_EPOC_TOTAL_LOSS_OF_PRECISION = -7,
		E_EPOC_BAD_HANDLE = -8,
		E_EPOC_OVERFLOW = -9,
		E_EPOC_UNDERFLOW = -10,
		E_EPOC_ALREADY_EXISTS = -11,
		E_EPOC_PATH_NOT_FOUND = -12,
		E_EPOC_DIED = -13,
		E_EPOC_IN_USE = -14,
		E_EPOC_SERVER_TERMINATED = -15,
		E_EPOC_SERVER_BUSY = -16,
		E_EPOC_COMPLETION = -17,
		E_EPOC_NOT_READY = -18,
		E_EPOC_UNKNOWN = -19,
		E_EPOC_CORRUPT = -20,
		E_EPOC_ACCESS_DENIED = -21,
		E_EPOC_LOCKED = -22,
		E_EPOC_WRITE = -23,
		E_EPOC_DISMOUNTED = -24,
		E_EPOC_EoF = -25,
		E_EPOC_DISK_FULL = -26,
		E_EPOC_BAD_DRIVER = -27,
		E_EPOC_BAD_NAME = -28,
		E_EPOC_COMMS_LINE_FAIL = -29,
		E_EPOC_COMMS_FRAME = -30,
		E_EPOC_COMMS_OVERRUN = -31,
		E_EPOC_COMMS_PARITY = -32,
		E_EPOC_TIMEOUT = -33,
		E_EPOC_COULD_NOT_CONNECT = -34,
		E_EPOC_COULD_NOT_DISCONNECT = -35,
		E_EPOC_DISCONNECTED = -36,
		E_EPOC_BAD_LIBRARY_ENTRY_POINT = -37,
		E_EPOC_BAD_DESCRIPTOR = -38,
		E_EPOC_ABORT = -39,
		E_EPOC_TOO_BIG = -40,
		E_EPOC_DIVIDE_BY_ZERO = -41,
		E_EPOC_BAD_POWER = -42,
		E_EPOC_DIR_FULL = -43
	};

	enum commands {
		CLOSE_HANDLE     = 0x01,
		OPEN_DIR         = 0x10,
		READ_DIR         = 0x12,
		GET_DRIVE_LIST   = 0x13,
		DRIVE_INFO       = 0x14,
		SET_VOLUME_LABEL = 0x15,
		OPEN_FILE        = 0x16,
		TEMP_FILE        = 0x17,
		READ_FILE        = 0x18,
		WRITE_FILE       = 0x19,
		SEEK_FILE        = 0x1a,
		DELETE           = 0x1b,
		REMOTE_ENTRY     = 0x1c,
		FLUSH            = 0x1d,
		SET_SIZE         = 0x1e,
		RENAME           = 0x1f,
		MK_DIR_ALL       = 0x20,
		RM_DIR           = 0x21,
		SET_ATT          = 0x22,
		ATT              = 0x23,
		SET_MODIFIED     = 0x24,
		MODIFIED         = 0x25,
		SET_SESSION_PATH = 0x26,
		SESSION_PATH     = 0x27,
		READ_WRITE_FILE  = 0x28,
		CREATE_FILE      = 0x29,
		REPLACE_FILE     = 0x2a,
		PATH_TEST        = 0x2b,
		LOCK             = 0x2d,
		UNLOCK           = 0x2e,
		OPEN_DIR_UID     = 0x2f,
		DRIVE_NAME       = 0x30,
		SET_DRIVE_NAME   = 0x31,
		REPLACE          = 0x32
	};

	const char *getConnectName();
	long err2psierr(long);

	// Communication
	bool sendCommand(enum commands, bufferStore &);
	long getResponse(bufferStore &);
	char *convertSlash(const char *);

	// time-conversion
	unsigned long micro2time(unsigned long, unsigned long);
	void time2micro(unsigned long, unsigned long &, unsigned long &);

	// Vars
	ppsocket *skt;
	int serNum;
	long status;
	int tDiff;
};

#endif
