#ifndef _rpcsfactory_h_
#define _rpcsfactory_h_

#include "rpcs.h"

class ppsocket;

class rpcsfactory {
	public:
		rpcsfactory(ppsocket * skt);
		virtual rpcs * create(bool);

	private:
		// Vars
		ppsocket *skt;
		int serNum;
};

#endif

