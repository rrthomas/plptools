#ifndef _iowatch_h
#define _iowatch_h

#include "bool.h"

/**
 * A simple wrapper for select()
 *
 * IOWatch is a simple wrapper for the select
 * system call. In particular, it takes care
 * of passing the maximum file descriptor
 * argument (arg 1) of select() correctly.
 * IOWatch handles select on read descriptors only.
 */
class IOWatch {
public:
	/**
	 * Creates a new instance.
	 */
	IOWatch();

	/**
	 * Destroys an instance.
	 */
	~IOWatch();
 
	/**
	 * Adds a file descriptor to
	 * the set of descriptors.
	 *
	 * @param fd The file descriptor to add.
	 */
	void addIO(const int fd);

	/**
	 * Removes a file descriptor from the
	 * set of descriptors.
	 *
	 * @param fd The file descriptor to remove.
	 */
	void remIO(const int fd);

	/**
	 * Performs a select() call.
	 *
	 * @param secs Number of seconds to wait.
	 * @param usecs Number of microseconds to wait.
	 *
	 * @return true, if any of the descriptors is
	 * 	readable.
	 */
	bool watch(const long secs, const long usecs);

private:
	int num;
	int *io;
};

#endif
