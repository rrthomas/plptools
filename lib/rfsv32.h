#ifndef _rfsv32_h_
#define _rfsv32_h_

class ppsocket;
class bufferStore;
class bufferArray;

class rfsv32 {
	public:
	rfsv32(ppsocket * skt);
	~rfsv32();

	long dir(const char *name, bufferArray * files);
	long dircount(const char *name, long *count);
	long copyFromPsion(const char *from, const char *to);
	long copyToPsion(const char *from, const char *to);
	long mkdir(const char *name);
	long rmdir(const char *name);
	long remove(const char *name);
	long rename(const char *oldname, const char *newname);
	long mktemp(long *handle, char *tmpname);
	long fgeteattr(const char *name, long *attr, long *size, long *time);
	long fgetattr(const char *name, long *attr);
	long fsetattr(const char *name, long seta, long unseta);
	long fgetmtime(const char *name, long *mtime);
	long fsetmtime(const char *name, long mtime);
	long fopendir(long attr, const char *name, long &handle);
	long fopen(long attr, const char *name, long &handle);
	long fcreatefile(long attr, const char *name, long &handle);
	long freplacefile(long attr, const char *name, long &handle);
	long fseek(long handle, long pos, long mode);
	long fread(long handle, char *buf, long len);
	long fwrite(long handle, char *buf, long len);
	long fsetsize(long handle, long size);
	long fclose(long handle);

	long devlist(long *devbits);
	char *devinfo(int devnum, long *vfree, long *vtotal, long *vattr, long *vuiqueid);
	long getStatus();
	char *opErr(long status);

	enum seek_mode {
		PSEEK_SET = 1,
		PSEEK_CUR = 2,
		PSEEK_END = 3
	};

	 private:
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

	enum file_attrib {
		READ_ONLY = 0x0001,
		HIDDEN = 0x0002,
		SYSTEM = 0x0004,
		DIRECTORY = 0x0010,
		ARCHIVE = 0x0020,
		VOLUME = 0x0040,
		NORMAL = 0x0080,
		TEMPORARY = 0x0100,
		COMPRESSED = 0x0800
	};

	enum open_mode {
		SHARE_EXCLUSIVE = 0x0000,
		SHARE_READERS = 0x0001,
		SHARE_ANY = 0x0002,
		BINARY = 0x0000,
		TEXT = 0x0020,
		READ_WRITE = 0x0200
	};

	enum errs {
		NONE = 0,
		NOT_FOUND = -1,
		GENERAL = -2,
		CANCEL = -3,
		NO_MEMORY = -4,
		NOT_SUPPORTED = -5,
		ARGUMENT = -6,
		TOTAL_LOSS_OF_PRECISION = -7,
		BAD_HANDLE = -8,
		OVERFLOW = -9,
		UNDERFLOW = -10,
		ALREADY_EXISTS = -11,
		PATH_NOT_FOUND = -12,
		DIED = -13,
		IN_USE = -14,
		SERVER_TERMINATED = -15,
		SERVER_BUSY = -16,
		COMPLETION = -17,
		NOT_READY = -18,
		UNKNOWN = -19,
		CORRUPT = -20,
		ACCESS_DENIED = -21,
		LOCKED = -22,
		WRITE = -23,
		DISMOUNTED = -24,
		EoF = -25,
		DISK_FULL = -26,
		BAD_DRIVER = -27,
		BAD_NAME = -28,
		COMMS_LINE_FAIL = -29,
		COMMS_FRAME = -30,
		COMMS_OVERRUN = -31,
		COMMS_PARITY = -32,
		PSI_TIMEOUT = -33,
		COULD_NOT_CONNECT = -34,
		COULD_NOT_DISCONNECT = -35,
		DISCONNECTED = -36,
		BAD_LIBRARY_ENTRY_POINT = -37,
		BAD_DESCRIPTOR = -38,
		ABORT = -39,
		TOO_BIG = -40,
		DIVIDE_BY_ZERO = -41,
		BAD_POWER = -42,
		DIR_FULL = -43
	};

	const char *getConnectName();

	// Communication
	bool sendCommand(enum commands c, bufferStore & data);
	long getResponse(bufferStore & data);
	void convertSlash(const char *name);

	// time-conversion
	unsigned long micro2time(unsigned long microHi, unsigned long microLo);
	void time2micro(unsigned long mtime, unsigned long &microHi, unsigned long &microLo);

	// Vars
	ppsocket *skt;
	int serNum;
	long status;
	int tDiff;
};

#endif
