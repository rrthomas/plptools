#include "log.h"

logbuf::logbuf(int _level) {
	ptr = buf;
	len = 0;
	level = _level;
}

int logbuf::overflow(int c) {
	if (c == '\n') {
		*ptr++ = '\n';
		*ptr = '\0';
		syslog(level, buf);
		ptr = buf;
		len = 0;
		return 0;
	}
	if ((len + 2) >= sizeof(buf))
		return EOF;
	*ptr++ = c;
	len++;
	return 0;
}
