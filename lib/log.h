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
#ifndef _LOG_H_
#define _LOG_H_

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
    * Called by the associated
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
    unsigned int len;

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

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
