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
#  define N_(x) (x)
#  define _(x) gettext(x)
#else
#  define N_(x) (x)
#  define _(x) (x)
#  define textdomain(x)
#endif

/* Define this, if you have gettext */
#define HAVE_GETTEXT 1

#endif /* _INTL_H_ */
