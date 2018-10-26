#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <climits>
#else
#include <stdint.h>
#include <limits.h>
#endif

// 8-bit types
#ifdef __CHAR_UNSIGNED__
typedef char    U8;
typedef int8_t  I8;
#else
typedef uint8_t U8;
typedef char    I8;
#endif

// other stuff
typedef void     U0;

typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int16_t  I16;
typedef int32_t  I32;
typedef int64_t  I64;

typedef float    F32;
typedef double   F64;

// unsigned
#define  U8_WIDTH 8
#define U16_WIDTH 16
#define U32_WIDTH 32
#define U64_WIDTH 64

#define  U8_BYTES 1
#define U16_BYTES 2
#define U32_BYTES 4
#define U64_BYTES 8

// signed
#define  I8_WIDTH 8
#define I16_WIDTH 16
#define I32_WIDTH 32
#define I64_WIDTH 64

#define  I8_BYTES 1
#define I16_BYTES 2
#define I32_BYTES 4
#define I64_BYTES 8

// floating
#define F32_WIDTH 32
#define F64_WIDTH 64

#define F32_BYTES 4
#define F64_BYTES 8

// pointers
#define PTR_WIDTH() (sizeof(U0 *) * U8_WIDTH)
#define PTR_BYTES() (sizeof(U0 *))

// misc
#define IGNORE(n) (U0) n // same as U0() but used to ignore function args

#if UINTPTR_MAX == 0xFFFFFFFF
#define BITS32
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
#define BITS64
#endif
