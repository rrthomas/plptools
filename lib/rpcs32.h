#ifndef _rpcs32_h_
#define _rpcs32_h_

#include "rpcs.h"

class ppsocket;

class rpcs32 : public rpcs {
	public:
		rpcs32(ppsocket *);
		~rpcs32();

		Enum<rfsv::errs> queryDrive(const char, bufferArray &);
		Enum<rfsv::errs> getCmdLine(const char *, bufferStore &); 
		Enum<rfsv::errs> getMachineInfo(machineInfo &);
		Enum<rfsv::errs> configOpen(void);
		Enum<rfsv::errs> configRead(void);
#if 0
		Enum<rfsv::errs> closeHandle(int);
		Enum<rfsv::errs> regOpenIter(void);
		Enum<rfsv::errs> regReadIter(void);
		Enum<rfsv::errs> regWrite(void);
		Enum<rfsv::errs> regRead(void);
		Enum<rfsv::errs> regDelete(void);
		Enum<rfsv::errs> setTime(void);
		Enum<rfsv::errs> configOpen(void);
		Enum<rfsv::errs> configRead(void);
		Enum<rfsv::errs> configWrite(void);
		Enum<rfsv::errs> queryOpen(void);
		Enum<rfsv::errs> queryRead(void);
		Enum<rfsv::errs> quitServer(void);
#endif

};

#endif
