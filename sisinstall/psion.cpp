
#include "psion.h"

#include <plpintl.h>
#include <rfsv.h>
#include <rpcs.h>
#include <rfsvfactory.h>
#include <rpcsfactory.h>
#include <bufferarray.h>
#include <bufferstore.h>
#include <ppsocket.h>

#include <dirent.h>
#include <netdb.h>

#include <stdio.h>

Psion::~Psion()
{
	disconnect();
}

bool
Psion::connect()
{
	int sockNum = DPORT;

#if 0
	setlocale (LC_ALL, "");
	textdomain(PACKAGE);
#endif

	struct servent *se = getservbyname("psion", "tcp");
	endservent();
	if (se != 0L)
		sockNum = ntohs(se->s_port);

#if 0
	// Command line parameter processing
	if ((argc > 2) && !strcmp(argv[1], "-p")) {
		sockNum = atoi(argv[2]);
		argc -= 2;
		for (int i = 1; i < argc; i++)
			argv[i] = argv[i + 2];
	}
#endif

	m_skt = new ppsocket();
	if (!m_skt->connect(NULL, sockNum)) {
		return false;
	}
	m_skt2 = new ppsocket();
	if (!m_skt2->connect(NULL, sockNum)) {
		return false;
	}
	m_rfsvFactory = new rfsvfactory(m_skt);
	m_rpcsFactory = new rpcsfactory(m_skt2);
	m_rfsv = m_rfsvFactory->create(true);
	m_rpcs = m_rpcsFactory->create(true);
	if ((m_rfsv != NULL) && (m_rpcs != NULL))
		return true;
	return false;
}

Enum<rfsv::errs>
Psion::copyToPsion(const char * const from, const char * const to,
				   void *, cpCallback_t func)
{
	return m_rfsv->copyToPsion(from, to, NULL, func);
}

Enum<rfsv::errs>
Psion::devinfo(const char drive, PlpDrive& plpDrive)
{
	return m_rfsv->devinfo(drive, plpDrive);
}

Enum<rfsv::errs>
Psion::devlist(u_int32_t& devbits)
{
	Enum<rfsv::errs> res;
	res = m_rfsv->devlist(devbits);
	return res;
}

bool
Psion::dirExists(const char* name)
{
	rfsvDirhandle handle;
	Enum<rfsv::errs> res;
	bool exists = false;
	res = m_rfsv->opendir(rfsv::PSI_A_ARCHIVE, name, handle);
	if (res == rfsv::E_PSI_GEN_NONE)
		exists = true;
	res = m_rfsv->closedir(handle);
	return exists;
}

void
Psion::disconnect()
{
	delete m_rfsv;
	delete m_rpcs;
	delete m_skt;
	delete m_skt2;
	delete m_rfsvFactory;
	delete m_rpcsFactory;
}

Enum<rfsv::errs>
Psion::mkdir(const char* dir)
{
	return m_rfsv->mkdir(dir);
}

