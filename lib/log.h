/* $Id$
 */

#ifndef _log_h_
#define _log_h_

#include <ostream.h>
#include <syslog.h>

class logbuf : public streambuf {
	public:
		logbuf(int);
		int overflow(int c = EOF);
	private:
		char *ptr;
		int len;
		int level;
		char buf[1024];
};

#endif
