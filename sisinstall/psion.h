#ifndef _PSION_H
#define _PSION_H

#include <Enum.h>
#include "rfsv.h"

class ppsocket;
class rfsvfactory;
class rpcsfactory;
class rpcs;

/**
 * Semi smart proxy for communicating with a Psion.
 */
class Psion
{
public:

	virtual ~Psion();

	virtual bool connect();

	virtual Enum<rfsv::errs> copyFromPsion(const char * const from, int fd,
										   cpCallback_t func);

    virtual Enum<rfsv::errs> copyToPsion(const char * const from,
										 const char * const to,
										 void *, cpCallback_t func);

	virtual Enum<rfsv::errs> devinfo(const char drive, PlpDrive& plpDrive);

	virtual Enum<rfsv::errs> devlist(u_int32_t& devbits);

	virtual Enum<rfsv::errs> dir(const char* dir, PlpDir& files);

	virtual bool dirExists(const char* name);

	virtual void disconnect();

	virtual Enum<rfsv::errs> mkdir(const char* dir);

	virtual void remove(const char* name);

private:

	ppsocket* m_skt;
	ppsocket* m_skt2;
	rfsvfactory* m_rfsvFactory;
	rpcsfactory* m_rpcsFactory;
	rpcs* m_rpcs;
	rfsv* m_rfsv;

};

#endif

