#ifndef _rpcs32_h_
#define _rpcs32_h_

#include "rpcs.h"

class ppsocket;

class rpcs32 : public rpcs {
	public:
		rpcs32(ppsocket *);
		~rpcs32();

		int queryDrive(const char, bufferArray &);
		int getCmdLine(const char *, bufferStore &); 
		int getMachineInfo(machineInfo &);
		int configOpen(void);
		int configRead(void);
#if 0
		int closeHandle(int);
		int regOpenIter(void);
		int regReadIter(void);
		int regWrite(void);
		int regRead(void);
		int regDelete(void);
		int setTime(void);
		int configOpen(void);
		int configRead(void);
		int configWrite(void);
		int queryOpen(void);
		int queryRead(void);
		int quitServer(void);
#endif

};

#endif
