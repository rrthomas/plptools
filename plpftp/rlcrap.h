/*
 * Woraround for crappy readline.h which defines wrong prototypes.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#if HAVE_LIBREADLINE
#include <readline/readline.h>
#ifdef __cplusplus
extern "C" {
#endif
extern CPFunction *cmdgen_ptr;
extern CPFunction *fnmgen_ptr;

typedef char *(*CorrectFunProto)(char *, int);

extern void rlcrap_setpointers(CorrectFunProto, CorrectFunProto);
#ifdef __cplusplus
}
#endif
#endif
