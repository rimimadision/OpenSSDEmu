#ifndef TYPE_H
#define TYPE_H

#include "../emu/emu_config.h"
#include "../lib/assert.h"

#define KB (1u << 10)
#define MB (1u << 20)
#define GB (1u << 30) // GB should be u32, do convert outside

/* 
 * for machine where code running on 
 * there is no difference between int32_t and uint32_t
 * it's only a perspective how code see it 
 * what's really matter is the length of variable
 * like u32 or u64
 * 
 * so num of things we need remember are 2
 * 
 * 1. always ensure your variable unsigned
 * 2. think twice for your usage of integer in a machine perspective 
 */
static_assert(sizeof(GB) == sizeof(1u), "GB should be u32, do convert outside");

#ifndef EMU
#include "xil_types.h"
#else
#define u64 unsigned long long 
#define u32 unsigned int
#define u16 unsigned short
#define u8 unsigned char
#define char8 char
#endif

#define boolean u8

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define FAIL (1)
#define SUCCESS (0)

#ifndef NULL
#define NULL (0)
#endif

#define DIV_ROUND_UP(N, M) ((N - 1) / M + 1)

#endif
