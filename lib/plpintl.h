/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
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
#ifndef _PLPINTL_H_
#define _PLPINTL_H_

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
static inline const char *X_(const char *t) { return gettext(t); }
static inline const char *_(const char *t) { return gettext(t); }
#else
static inline const char *X_(const char *t) { return t; }
static inline const char *_(const char *t) { return t; }
#  define textdomain(x)
#endif
static inline const char *N_(const char *t) { return t; } 

/* Define this, if you have gettext */
#define HAVE_GETTEXT 1

#endif /* _INTL_H_ */

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
