#ifndef _PSION_H
#define _PSION_H

#include <Enum.h>
#include "rfsv.h"

class ppsocket;
class rfsvfactory;
class rpcsfactory;
class rpcs;

class Psion
{
public:

	virtual ~Psion();

	bool connect();

	Enum<rfsv::errs> devlist(u_int32_t& devbits);

	void disconnect();

	rfsv* m_rfsv;

private:

	ppsocket* m_skt;
	ppsocket* m_skt2;
	rfsvfactory* m_rfsvFactory;
	rpcsfactory* m_rpcsFactory;
	rpcs* m_rpcs;

};

#endif

