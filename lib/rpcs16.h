#ifndef _rpcs16_h_
#define _rpcs16_h_

#include "rpcs.h"

class ppsocket;
class bufferStore;
class rpcsfactory;

/**
 * This is the implementation of the @ref rpcs protocol for
 * Psion series 3 (SIBO) variant.  You normally never create
 * objects of this class directly. Thus the constructor is
 * private. Use @ref rpcsfactory for creating an instance of
 * @ref rpcs . For a complete documentation, see @ref rpcs .
 */
class rpcs16 : public rpcs {
	friend rpcsfactory;

 public:
	~rpcs16();
	
	Enum<rfsv::errs> queryDrive(const char, bufferArray &);
	Enum<rfsv::errs> getCmdLine(const char *, bufferStore &); 
 private:
	rpcs16(ppsocket *);
};

#endif
