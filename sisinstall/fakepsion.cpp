
#include "fakepsion.h"

#include <stdio.h>

FakePsion::~FakePsion()
{
}

bool
FakePsion::connect()
{
	return true;
}

Enum<rfsv::errs>
FakePsion::copyToPsion(const char * const from, const char * const to,
				   void *, cpCallback_t func)
{
	printf(" -- Not really copying %s to %s\n", from, to);
	return rfsv::E_PSI_GEN_NONE;
}

Enum<rfsv::errs>
FakePsion::devinfo(const char drive, PlpDrive& plpDrive)
{
	return rfsv::E_PSI_GEN_NONE;
}

Enum<rfsv::errs>
FakePsion::devlist(u_int32_t& devbits)
{
	return rfsv::E_PSI_GEN_FAIL;
}

bool
FakePsion::dirExists(const char* name)
{
	return true;
}

void
FakePsion::disconnect()
{
}

Enum<rfsv::errs>
FakePsion::mkdir(const char* dir)
{
	printf(" -- Not really creating dir %s\n", dir);
	return rfsv::E_PSI_GEN_NONE;
}

