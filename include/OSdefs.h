#ifdef __FreeBSD__
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <stdio.h>
#endif

#if defined(__NeXT__)
# include <libc.h>
# include <string.h>
# include <objc/hashtable.h>
# define strdup NXCopyStringBuffer
#endif
