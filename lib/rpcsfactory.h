#ifndef _rpcsfactory_h_
#define _rpcsfactory_h_

#include "rpcs.h"

class ppsocket;

/**
 * A factory for automatically instantiating the correct protocol
 * variant depending on the connected Psion.
 */
class rpcsfactory {
 public:
	/**
	 * Constructs a rpcsfactory.
	 *
	 * @param skt The socket to be used for connecting
	 * to the ncpd daemon.
	 */
	rpcsfactory(ppsocket * skt);

	/**
	 * Creates a new rpcs instance.
	 *
	 * @param reconnect Set to true, if automatic reconnect
	 * should be performed on failure.
	 *
	 * @returns A pointer to a newly created rpcs instance or
	 * NULL on failure.
	 */
	virtual rpcs * create(bool reconnect);

 private:
	/**
	 * The socket to be used for connecting to the
	 * ncpd daemon.
	 */
	ppsocket *skt;
};

#endif

