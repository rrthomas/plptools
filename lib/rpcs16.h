#ifndef _rpcs16_h_
#define _rpcs16_h_

#include "rpcs.h"

class ppsocket;
class bufferStore;

class rpcs16 : public rpcs {
	public:
		rpcs16(ppsocket *);
		~rpcs16();

		int queryDrive(const char, bufferArray &);
		int getCmdLine(const char *, bufferStore &); 
};

#endif
