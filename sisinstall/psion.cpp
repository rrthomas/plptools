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
		return 1;
	}
	m_skt2 = new ppsocket();
	if (!m_skt2->connect(NULL, sockNum)) {
		return 1;
	}
	m_rfsvFactory = new rfsvfactory(m_skt);
	m_rpcsFactory = new rpcsfactory(m_skt2);
	m_rfsv = m_rfsvFactory->create(true);
	m_rpcs = m_rpcsFactory->create(true);
	if ((m_rfsv != NULL) && (m_rpcs != NULL))
		return true;
}

Psion::~Psion()
{
	disconnect();
}

Enum<rfsv::errs>
Psion::devlist(u_int32_t& devbits)
{
	printf("Running devlist\n");
	u_int32_t devb;
	Enum<rfsv::errs> res;
	res = m_rfsv->devlist(devb);
	devbits = devb;
	return res;
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

