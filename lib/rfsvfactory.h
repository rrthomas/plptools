#ifndef _rfsvfactory_h_
#define _rfsvfactory_h_

#include "rfsv.h"

class ppsocket;

class rfsvfactory {
	public:
	rfsvfactory(ppsocket * skt);
	virtual rfsv * create(bool);

	private:
	// Vars
	ppsocket *skt;
	int serNum;
};

#endif

