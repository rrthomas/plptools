#ifndef _INTTYPES_H_
#define _INTTYPES_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>

#include <stdint.h>

#ifndef GNU_INTTYPES
typedef uint8_t  u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;
typedef int64_t  s_int64_t;
#endif // GNU_INTTYPES

#endif // _INTTYPES_H_
