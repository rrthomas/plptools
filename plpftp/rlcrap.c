/*
 * Woraround for crappy readline.h which defines wrong prototypes.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#if HAVE_LIBREADLINE
#include <stdio.h>
#include "rlcrap.h"

CPFunction *cmdgen_ptr;
CPFunction *fnmgen_ptr;

void rlcrap_setpointers(CorrectFunProto cgp, CorrectFunProto fgp) {
	cmdgen_ptr = cgp;
	fnmgen_ptr = fgp;
}
#endif
