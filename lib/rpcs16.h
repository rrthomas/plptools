#ifndef _rpcs16_h_
#define _rpcs16_h_

#include "rpcs.h"

class ppsocket;
class bufferStore;

class rpcs16 : public rpcs {
	public:
		rpcs16(ppsocket *);
		~rpcs16();

		Enum<rfsv::errs> queryDrive(const char, bufferArray &);
		Enum<rfsv::errs> getCmdLine(const char *, bufferStore &); 
};

#endif
