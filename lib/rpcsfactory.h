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
	 * The known errors which can happen during @ref create .
	 */
	enum errs {
		FACERR_NONE = 0,
		FACERR_COULD_NOT_SEND = 1,
		FACERR_AGAIN = 2,
		FACERR_NOPSION = 3,
		FACERR_PROTVERSION = 4,
		FACERR_NORESPONSE = 5,
	};

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

	/**
	 * Retrieve an error code.
	 *
	 * @returns The error code, in case @ref create has
	 * failed, 0 otherwise.
	 */
	virtual Enum<errs> getError() { return err; }

 private:
	/**
	 * The socket to be used for connecting to the
	 * ncpd daemon.
	 */
	ppsocket *skt;
	Enum<errs> err;
};

#endif

