#ifndef _rpcs_h_
#define _rpcs_h_

class ppsocket;
class bufferStore;
class bufferArray;

class rpcs {
	public:
		virtual ~rpcs() {};
		void reset();
		void reconnect();
		long getStatus();

		// API idendical on SIBO and EPOC
		int getNCPversion(int &, int &);
		int execProgram(const char *, const char *);
		int stopProgram(const char *);
		int queryProgram(const char *);
		int formatOpen(const char *, int &, int &);
		int formatRead(int);
		int getUniqueID(const char *, long &);
		int getOwnerInfo(bufferArray &);
		int getMachineType(int &);
		int fuser(const char *, char *, int);
		int quitServer(void);

		// API different on SIBO and EPOC
		virtual int queryDrive(const char, bufferArray &) = 0;
		virtual int getCmdLine(const char *, char *, int) = 0; 

		// API only existent on EPOC
		// default-methods for SIBO here.
		virtual int getMachineInfo(void) { return E_PSI_NOT_SIBO;}
		virtual int closeHandle(int) { return E_PSI_NOT_SIBO;}
		virtual int regOpenIter(void) { return E_PSI_NOT_SIBO;}
		virtual int regReadIter(void) { return E_PSI_NOT_SIBO;}
		virtual int regWrite(void) { return E_PSI_NOT_SIBO;}
		virtual int regRead(void) { return E_PSI_NOT_SIBO;}
		virtual int regDelete(void) { return E_PSI_NOT_SIBO;}
		virtual int setTime(void) { return E_PSI_NOT_SIBO;}
		virtual int configOpen(void) { return E_PSI_NOT_SIBO;}
		virtual int configRead(void) { return E_PSI_NOT_SIBO;}
		virtual int configWrite(void) { return E_PSI_NOT_SIBO;}
		virtual int queryOpen(void) { return E_PSI_NOT_SIBO;}
		virtual int queryRead(void) { return E_PSI_NOT_SIBO;}

		enum errs {
            E_PSI_GEN_NONE = 0,
			E_PSI_GEN_FAIL = -1,
            E_PSI_FILE_DISC = -50,
			// Special error code for "Operation not permitted in RPCS16"
            E_PSI_NOT_SIBO = -200
		};

		enum machs {
			PSI_MACH_UNKNOWN = 0,
			PSI_MACH_PC = 1,
			PSI_MACH_MC = 2,
			PSI_MACH_HC = 3,
			PSI_MACH_S3 = 4,
			PSI_MACH_S3A = 5,
			PSI_MACH_WORKABOUT = 6,
			PSI_MACH_SIENNA = 7,
			PSI_MACH_S3C = 8,
			PSI_MACH_S5 = 32,
			PSI_MACH_WINC = 33,
//TODO: Code for 5mx
		};

	protected:

		// Vars
		ppsocket *skt;
		long status;

		enum commands {
			QUERY_NCP        = 0x00,
			EXEC_PROG        = 0x01,
			QUERY_DRIVE      = 0x02,
			STOP_PROG        = 0x03,
			QUERY_PROG       = 0x04,
			FORMAT_OPEN      = 0x05,
			FORMAT_READ      = 0x06,
			GET_UNIQUEID     = 0x07,
			GET_OWNERINFO    = 0x08,
			GET_MACHINETYPE  = 0x09,
			GET_CMDLINE      = 0x0a,
			FUSER            = 0x0b,
			CONFIG_OPEN      = 0x66,
			CONFIG_READ      = 0x6d,
			QUIT_SERVER      = 0xff
		};

		// Communication
		bool sendCommand(enum commands, bufferStore &);
		long getResponse(bufferStore &);
		const char *getConnectName();

		char *convertSlash(const char *);
};

#endif
