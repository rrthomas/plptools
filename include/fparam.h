/* $Id$
 *
 */

#ifndef _fparam_h_
#define _fparam_h_

#if defined(sun) && defined(__STDC__) && !defined(__SVR4)
# include "sun_stdlib.h"
# define __P(a) a
#endif

#ifndef __P
#  if defined(__STDC__)
#    define __P(a) a
#  else
#    define __P(a) ()
#  endif
#endif

#endif
