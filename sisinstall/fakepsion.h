#ifndef _FAKEPSION_H
#define _FAKEPSION_H

#include "psion.h"

/**
 * A dummy version of the Psion proxy, mainly for testing the installer.
 */
class FakePsion : public Psion
{
public:

	virtual ~FakePsion();

	virtual bool connect();

    virtual Enum<rfsv::errs> copyToPsion(const char * const from,
										 const char * const to,
										 void *, cpCallback_t func);

	virtual Enum<rfsv::errs> devinfo(const char drive, PlpDrive& plpDrive);

	virtual Enum<rfsv::errs> devlist(u_int32_t& devbits);

	virtual Enum<rfsv::errs> dir(const char* dir, PlpDir& files);

	virtual bool dirExists(const char* name);

	virtual void disconnect();

	virtual Enum<rfsv::errs> mkdir(const char* dir);

};

#endif

