#ifndef _rfsvfactory_h_
#define _rfsvfactory_h_

#include "rfsv.h"

class ppsocket;

/**
 * A factory for automatically instantiating the correct
 * @ref rfsv protocol variant depending on the connected Psion.
 */
class rfsvfactory {
 public:
	/**
	 * Constructs a rfsvfactory.
	 *
	 * @param skt The socket to be used for connecting
	 * to the ncpd daemon.
	 */
	rfsvfactory(ppsocket * skt);

	/**
	 * Creates a new @ref rfsv instance.
	 *
	 * @param reconnect Set to true, if automatic reconnect
	 * should be performed on failure.
	 *
	 * @returns A pointer to a newly created rfsv instance or
	 * NULL on failure.
	 */
	virtual rfsv * create(bool);

 private:
	/**
	 * The socket to be used for connecting to the
	 * ncpd daemon.
	 */
	ppsocket *skt;
	int serNum;
};

#endif

