/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999  Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "log.h"
#include <unistd.h>

logbuf::logbuf(int level, int fd) {
    ptr = buf;
    len = 0;
    _on = true;
    _level = level;
    _fd = fd;
}

int logbuf::overflow(int c) {
    if (c == '\n') {
	*ptr++ = '\n';
	*ptr = '\0';
	if (_on)
	    syslog(_level, buf);
	else if (_fd != -1)
	    write(_fd, buf, len + 1);
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

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
