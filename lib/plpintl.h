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
#ifndef _INTL_H_
#define _INTL_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_STPCPY
#include <string.h>

extern inline char * stpcpy(char *dest, const char *src) {
	char c;
	do {
		c = *dest++ = *src++;
	} while (c);
	return dest;
}
#endif

#if defined(ENABLE_NLS) && defined(HAVE_GETTEXT)
#  include <libintl.h>
#  define X_(x) gettext(x)
#  define N_(x) (x)
#  define _(x) gettext(x)
#else
#  define X_(x) (x)
#  define N_(x) (x)
#  define _(x) (x)
#  define textdomain(x)
#endif

/* Define this, if you have gettext */
#define HAVE_GETTEXT 1

#endif /* _INTL_H_ */

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
