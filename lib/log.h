/* $Id$
 */

#ifndef _log_h_
#define _log_h_

#include <ostream.h>
#include <syslog.h>

/**
 * A streambuffer, logging via syslog
 *
 * logbuf can be used, if you want to use syslog for
 * logging but don't want to change all your nice
 * C++-style output statements in your code.
 *
 * Here is an example showing the usage of logbuf:
 *
 * <PRE>
 *	openlog("myDaemon", LOG_CONS|LOG_PID, LOG_DAEMON);
 *	logbuf ebuf(LOG_ERR);
 *	ostream lerr(&ebuf);
 *
 *	... some code ...
 *
 *	lerr << "Whoops, got an error" << endl;
 * </PRE>
 */
class logbuf : public streambuf {
	public:

		/**
		 * Constructs a new instance.
		 *
		 * @param level The log level for this instance.
		 * 	see syslog(3) for symbolic names to use.
		 */
		logbuf(int level);

		/**
		 * @internal Called by the associated
		 * ostream to write a character.
		 * Stores the character in a buffer
		 * and calls syslog(level, buffer)
		 * whenever a LF is seen.
		 */
		int overflow(int c = EOF);

	private:

		/**
		 * Pointer to next char in buffer.
		 */
		char *ptr;

		/**
		 * Current length of buffer.
		 */
		int len;

		/**
		 * The log level to use with syslog.
		 */
		int level;

		/**
		 * The internal buffer for holding
		 * messages.
		 */
		char buf[1024];
};

#endif
