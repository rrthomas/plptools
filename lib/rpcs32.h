#ifndef _rpcs32_h_
#define _rpcs32_h_

#include "rpcs.h"

class ppsocket;
class rpcsfactory;

/**
 * This is the implementation of the @ref rpcs protocol for
 * Psion series 5 (EPOC) variant. You normally never create
 * objects of this class directly. Thus the constructor is
 * private. Use @ref rpcsfactory for creating an instance of
 * @ref rpcs . For a complete documentation, see @ref rpcs .
 */
class rpcs32 : public rpcs {
	friend rpcsfactory;

 public:
	~rpcs32();

	Enum<rfsv::errs> queryDrive(const char, bufferArray &);
	Enum<rfsv::errs> getCmdLine(const char *, bufferStore &); 
	Enum<rfsv::errs> getMachineInfo(machineInfo &);
	Enum<rfsv::errs> configOpen(void);
	Enum<rfsv::errs> configRead(void);
#if 0
	Enum<rfsv::errs> closeHandle(int);
#endif
	Enum<rfsv::errs> regOpenIter(void);
#if 0
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
 private:
	rpcs32(ppsocket *);
};

#endif
