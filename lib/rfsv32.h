#ifndef _rfsv32_h_
#define _rfsv32_h_

class ppsocket;
class bufferStore;
class bufferArray;

class rfsv32 {
	public:
	rfsv32(ppsocket * skt);
	~rfsv32();
	void reset();
	void reconnect();

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
		PSI_SEEK_SET = 1,
		PSI_SEEK_CUR = 2,
		PSI_SEEK_END = 3
	};

	enum file_attrib {
		PSI_ATTR_RONLY      = 0x0001,
		PSI_ATTR_HIDDEN     = 0x0002,
		PSI_ATTR_SYSTEM     = 0x0004,
		PSI_ATTR_DIRECTORY  = 0x0010,
		PSI_ATTR_ARCHIVE    = 0x0020,
		PSI_ATTR_VOLUME     = 0x0040,
		PSI_ATTR_NORMAL     = 0x0080,
		PSI_ATTR_TEMPORARY  = 0x0100,
		PSI_ATTR_COMPRESSED = 0x0800
	};

	enum open_mode {
		PSI_OMODE_SHARE_EXCLUSIVE = 0x0000,
		PSI_OMODE_SHARE_READERS = 0x0001,
		PSI_OMODE_SHARE_ANY = 0x0002,
		PSI_OMODE_BINARY = 0x0000,
		PSI_OMODE_TEXT = 0x0020,
		PSI_OMODE_READ_WRITE = 0x0200
	};

	enum errs {
		PSI_ERR_NONE = 0,
		PSI_ERR_NOT_FOUND = -1,
		PSI_ERR_GENERAL = -2,
		PSI_ERR_CANCEL = -3,
		PSI_ERR_NO_MEMORY = -4,
		PSI_ERR_NOT_SUPPORTED = -5,
		PSI_ERR_ARGUMENT = -6,
		PSI_ERR_TOTAL_LOSS_OF_PRECISION = -7,
		PSI_ERR_BAD_HANDLE = -8,
		PSI_ERR_OVERFLOW = -9,
		PSI_ERR_UNDERFLOW = -10,
		PSI_ERR_ALREADY_EXISTS = -11,
		PSI_ERR_PATH_NOT_FOUND = -12,
		PSI_ERR_DIED = -13,
		PSI_ERR_IN_USE = -14,
		PSI_ERR_SERVER_TERMINATED = -15,
		PSI_ERR_SERVER_BUSY = -16,
		PSI_ERR_COMPLETION = -17,
		PSI_ERR_NOT_READY = -18,
		PSI_ERR_UNKNOWN = -19,
		PSI_ERR_CORRUPT = -20,
		PSI_ERR_ACCESS_DENIED = -21,
		PSI_ERR_LOCKED = -22,
		PSI_ERR_WRITE = -23,
		PSI_ERR_DISMOUNTED = -24,
		PSI_ERR_EoF = -25,
		PSI_ERR_DISK_FULL = -26,
		PSI_ERR_BAD_DRIVER = -27,
		PSI_ERR_BAD_NAME = -28,
		PSI_ERR_COMMS_LINE_FAIL = -29,
		PSI_ERR_COMMS_FRAME = -30,
		PSI_ERR_COMMS_OVERRUN = -31,
		PSI_ERR_COMMS_PARITY = -32,
		PSI_ERR_TIMEOUT = -33,
		PSI_ERR_COULD_NOT_CONNECT = -34,
		PSI_ERR_COULD_NOT_DISCONNECT = -35,
		PSI_ERR_DISCONNECTED = -36,
		PSI_ERR_BAD_LIBRARY_ENTRY_POINT = -37,
		PSI_ERR_BAD_DESCRIPTOR = -38,
		PSI_ERR_ABORT = -39,
		PSI_ERR_TOO_BIG = -40,
		PSI_ERR_DIVIDE_BY_ZERO = -41,
		PSI_ERR_BAD_POWER = -42,
		PSI_ERR_DIR_FULL = -43
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

	const char *getConnectName();

	// Communication
	bool sendCommand(enum commands c, bufferStore & data);
	long getResponse(bufferStore & data);
	char *convertSlash(const char *name);

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
