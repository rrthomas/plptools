#ifndef _rfsv_h_
#define _rfsv_h_

class ppsocket;
class bufferStore;
class bufferArray;

#define RFSV_SENDLEN 2000

typedef int (*cpCallback_t)(long);

// Abstract base class of RFSV ; 16-bit and 32-bit variants implement this
// interface
class rfsv {
	public:
		virtual ~rfsv() {}
		virtual void reset() = 0;
		virtual void reconnect() = 0;
		virtual long getStatus() = 0;
		virtual const char *getConnectName() = 0;
		virtual long fopen(long, const char *, long &) = 0;
		virtual long mktemp(long *, char *) = 0;
		virtual long fcreatefile(long, const char *, long &) = 0;
		virtual long freplacefile(long, const char *, long &) = 0;
		virtual long fopendir(long, const char *, long &) = 0;
		virtual long fclose(long) = 0;
		virtual long dir(const char *, bufferArray *) = 0;
		virtual long fgetmtime(const char *, long *) = 0;
		virtual long fsetmtime(const char *, long) = 0;
		virtual long fgetattr(const char *, long *) = 0;
		virtual long fgeteattr(const char *, long *, long *, long *) =0; 
		virtual long fsetattr(const char *, long, long) = 0;
		virtual long dircount(const char *, long *) = 0;
		virtual long devlist(long *) = 0;
		virtual char *devinfo(int, long *, long *, long *, long *) = 0;
		virtual char *opAttr(long) = 0;
		virtual long opMode(long) = 0;
		virtual long fread(long, unsigned char *, long) = 0;
		virtual long fwrite(long, unsigned char *, long) = 0;
		virtual long copyFromPsion(const char *, const char *, cpCallback_t) = 0;
		virtual long copyToPsion(const char *, const char *, cpCallback_t) = 0;
		virtual long fsetsize(long, long) = 0;
		virtual long fseek(long, long, long) = 0;
		virtual long mkdir(const char *) = 0;
		virtual long rmdir(const char *) = 0;
		virtual long rename(const char *, const char *) = 0;
		virtual long remove(const char *) = 0;

		char *opErr(long);

		enum seek_mode {
			PSI_SEEK_SET = 1,
			PSI_SEEK_CUR = 2,
			PSI_SEEK_END = 3
		};

		enum open_flags {
			PSI_O_RDONLY = 00,
			PSI_O_WRONLY = 01,
			PSI_O_RDWR = 02,
		};

		enum open_mode {
			PSI_O_CREAT = 0100,
			PSI_O_EXCL = 0200,
			PSI_O_TRUNC = 01000,
			PSI_O_APPEND = 02000,
		};

		enum errs {
			E_PSI_GEN_NONE	= 0,
			E_PSI_GEN_FAIL = -1,
			E_PSI_GEN_ARG = -2,
			E_PSI_GEN_OS = -3,
			E_PSI_GEN_NSUP = -4,
			E_PSI_GEN_UNDER = -5,
			E_PSI_GEN_OVER = -6,
			E_PSI_GEN_RANGE = -7,
			E_PSI_GEN_DIVIDE = -8,
			E_PSI_GEN_INUSE = -9,
			E_PSI_GEN_NOMEMORY = - 10,
			E_PSI_GEN_NOSEGMENTS = -11,
			E_PSI_GEN_NOSEM = -12,
			E_PSI_GEN_NOPROC = -13,
			E_PSI_GEN_OPEN = -14,
			E_PSI_GEN_NOTOPEN = -15,
			E_PSI_GEN_IMAGE = -16,
			E_PSI_GEN_RECEIVER = -17,
			E_PSI_GEN_DEVICE = -18,
			E_PSI_GEN_FSYS = -19,
			E_PSI_GEN_START = -20,
			E_PSI_GEN_NOFONT = -21,
			E_PSI_GEN_TOOWIDE = -22,
			E_PSI_GEN_TOOMANY = -23,
			E_PSI_FILE_EXIST = -32,
			E_PSI_FILE_NXIST = -33,
			E_PSI_FILE_WRITE = -34,
			E_PSI_FILE_READ = -35,
			E_PSI_FILE_EOF = -36,
			E_PSI_FILE_FULL = -37,
			E_PSI_FILE_NAME = -38,
			E_PSI_FILE_ACCESS = -39,
			E_PSI_FILE_LOCKED = -40,
			E_PSI_FILE_DEVICE = -41,
			E_PSI_FILE_DIR = -42,
			E_PSI_FILE_RECORD = -43,
			E_PSI_FILE_RDONLY = -44,
			E_PSI_FILE_INV = -45,
			E_PSI_FILE_PENDING = -46,
			E_PSI_FILE_VOLUME = -47,
			E_PSI_FILE_CANCEL = -48,
			E_PSI_FILE_ALLOC = -49,
			E_PSI_FILE_DISC = -50,
			E_PSI_FILE_CONNECT = -51,
			E_PSI_FILE_RETRAN = -52,
			E_PSI_FILE_LINE = -53,
			E_PSI_FILE_INACT = -54,
			E_PSI_FILE_PARITY = -55,
			E_PSI_FILE_FRAME = -56,
			E_PSI_FILE_OVERRUN = -57,
			E_PSI_MDM_CONFAIL = -58,
			E_PSI_MDM_BUSY = -59,
			E_PSI_MDM_NOANS = -60,
			E_PSI_MDM_BLACKLIST = -61,
			E_PSI_FILE_NOTREADY = -62,
			E_PSI_FILE_UNKNOWN = -63,
			E_PSI_FILE_DIRFULL = -64,
			E_PSI_FILE_PROTECT = -65,
			E_PSI_FILE_CORRUPT = -66,
			E_PSI_FILE_ABORT = -67,
			E_PSI_FILE_ERASE = -68,
			E_PSI_FILE_INVALID = -69,
			E_PSI_GEN_POWER = -100,
			E_PSI_FILE_TOOBIG = -101,
			E_PSI_GEN_DESCR = -102,
			E_PSI_GEN_LIB = -103,
			E_PSI_FILE_NDISC = -104,
			E_PSI_FILE_DRIVER = -105,
			E_PSI_FILE_COMPLETION = -106,
			E_PSI_GEN_BUSY = -107,
			E_PSI_GEN_TERMINATED = -108,
			E_PSI_GEN_DIED = -109,
			E_PSI_FILE_HANDLE = -110,

			// Special error code for "Operation not permitted in RFSV16"
			E_PSI_NOT_SIBO = -200
		};
};

#endif

