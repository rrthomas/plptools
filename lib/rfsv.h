#ifndef _rfsv_h_
#define _rfsv_h_

#include "Enum.h"

class ppsocket;
class bufferStore;
class bufferArray;

const long RFSV_SENDLEN = 2000;

/**
 * Defines the callback procedure for
 * progress indication of copy operations.
 */
typedef int (*cpCallback_t)(long);

// Abstract base class of RFSV ; 16-bit and 32-bit variants implement this
// interface
/**
 * Access remote file services of a Psion.
 *
 * rfsv provides an API for accessing file services
 * of a Psion connected via ncpd. This class defines the
 * interface and a small amount of common constants and
 * methods. The majority of implementation is provided
 * by @ref rfsv32 and @ref rfsv16, which implement the
 * variations of the protocol for EPOC and SIBO respectively.
 * Usually, the class @ref rfsvfactory is used to instantiate
 * the correct variant depending on the remote machine,
 * currently connected.
 */
class rfsv {
	public:
		/**
		 * The kown modes for seek.
		 */
		enum seek_mode {
			PSI_SEEK_SET = 1,
			PSI_SEEK_CUR = 2,
			PSI_SEEK_END = 3
		};

		/**
		 * The known modes for file open.
		 */
		enum open_flags {
			PSI_O_RDONLY = 00,
			PSI_O_WRONLY = 01,
			PSI_O_RDWR = 02,
		};

		/**
		 * The known modes for file creation.
		 */
		enum open_mode {
			PSI_O_CREAT = 0100,
			PSI_O_EXCL = 0200,
			PSI_O_TRUNC = 01000,
			PSI_O_APPEND = 02000,
		};

		/**
		 * The known error codes.
		 */
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
			E_PSI_NOT_SIBO = -200,
			// Special error code for "internal library error"
			E_PSI_INTERNAL = -201
		};

		/**
		 * The known file attributes
		 */
		enum file_attribs {
			/**
			 * Attributes, valid on both EPOC and SIBO.
			 */
			PSI_A_RDONLY = 0x0001,
			PSI_A_HIDDEN = 0x0002,
			PSI_A_SYSTEM = 0x0004,
			PSI_A_DIR = 0x0008,
			PSI_A_ARCHIVE = 0x0010,
			PSI_A_VOLUME = 0x0020,

			/**
			 * Attributes, valid on EPOC only.
			 */
			PSI_A_NORMAL = 0x0040,
			PSI_A_TEMP = 0x0080,
			PSI_A_COMPRESSED = 0x0100,

			/**
			 * Attributes, valid on SIBO only.
			 */
			PSI_A_READ = 0x0200,
			PSI_A_EXEC = 0x0400,
			PSI_A_STREAM = 0x0800,
			PSI_A_TEXT = 0x1000
		};
		virtual ~rfsv() {}
		virtual void reset() = 0;
		virtual void reconnect() = 0;
		virtual Enum<errs> getStatus() = 0;
		virtual const char *getConnectName() = 0;
		virtual Enum<errs> fopen(long, const char *, long &) = 0;
		virtual Enum<errs> mktemp(long *, char *) = 0;
		virtual Enum<errs> fcreatefile(long, const char *, long &) = 0;
		virtual Enum<errs> freplacefile(long, const char *, long &) = 0;
		virtual Enum<errs> fopendir(long, const char *, long &) = 0;
		virtual Enum<errs> fclose(long) = 0;
		virtual Enum<errs> dir(const char *, bufferArray *) = 0;
		virtual Enum<errs> fgetmtime(const char *, long *) = 0;
		virtual Enum<errs> fsetmtime(const char *, long) = 0;
		virtual Enum<errs> fgetattr(const char *, long *) = 0;
		virtual Enum<errs> fgeteattr(const char *, long *, long *, long *) =0; 
		virtual Enum<errs> fsetattr(const char *, long, long) = 0;
		virtual Enum<errs> dircount(const char *, long *) = 0;
		virtual Enum<errs> devlist(long *) = 0;
		virtual char *devinfo(int, long *, long *, long *, long *) = 0;
		virtual char *opAttr(long) = 0;
		virtual long opMode(long) = 0;
		virtual long fread(long, unsigned char *, long) = 0;
		virtual long fwrite(long, unsigned char *, long) = 0;
		virtual Enum<errs> copyFromPsion(const char *, const char *, cpCallback_t) = 0;
		virtual Enum<errs> copyToPsion(const char *, const char *, cpCallback_t) = 0;
		virtual Enum<errs> fsetsize(long, long) = 0;
		virtual long fseek(long, long, long) = 0;
		virtual Enum<errs> mkdir(const char *) = 0;
		virtual Enum<errs> rmdir(const char *) = 0;
		virtual Enum<errs> rename(const char *, const char *) = 0;
		virtual Enum<errs> remove(const char *) = 0;

		virtual long attr2std(long) = 0;
		virtual long std2attr(long) = 0;
		
		char *opErr(long);
};

#endif

