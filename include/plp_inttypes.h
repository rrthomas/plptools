#ifndef _INTTYPES_H_
#define _INTTYPES_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef HAVE_SYS_INT_TYPES_H
#include <sys/int_types.h>
#endif

#ifndef GNU_INTTYPES
#ifdef ISO_INTTYPES
typedef uint8_t  u_int8_t;
typedef uint16_t u_int16_t;
typedef uint32_t u_int32_t;
typedef uint64_t u_int64_t;
typedef int64_t  s_int64_t;
#else
/* Last resort, declare ourselves */
typedef unsigned char      u_int8_t;
typedef unsigned short     u_int16_t;
typedef unsigned int       u_int32_t;
typedef unsigned long long u_int64_t;
typedef signed long long   s_int64_t;
#endif // ISO_INTTYPES
#endif // GNU_INTTYPES

#endif // _INTTYPES_H_
