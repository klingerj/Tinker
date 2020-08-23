#pragma once

#include <stdint.h>
#include <math.h>
#include <cmath>

#define TINKER_ASSERTS_ON

#ifdef TINKER_ASSERTS_ON
#include <assert.h>
#define TINKER_ASSERT(cond) assert((cond))
#else
#define TINKER_ASSERT(cond)
#endif

// C++ only!
#define RESTRICT __restrict

#define TINKER_INVALID_HANDLE 0xffffffff

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;

#define SIZE_1MB 1024
#define SIZE_2MB 1024 * 2

#define MAX(a, b) ((a > b) ? a : b)
#define MIN(a, b) ((a < b) ? a : b)
#define CLAMP(a, b, c) MIN(MAX(a, b), c)

#define ISPOW2(x) x && ((x & (x - 1)) == 0)

template <typename T>
T LOG2(T x)
{
    T i = 1;
    for (; x >> i; ++i) {}
    return i;
}

template <typename T>
T POW2(T x)
{
    return 1 << LOG2(x);
}
#define POW2_ROUNDUP(x) ISPOW2(x) ? x : POW2(x);

#define FLOAT_EQUAL(a, b) fabs(a - b) < FLT_EPSILON

inline uint32 SafeTruncateUint64(uint64 value)
{
    TINKER_ASSERT(value <= 0xffffffff);
    return (uint32)value;
}

#ifdef _WIN32
#define OPTIMIZATIONS_ON __pragma(optimize( "", on ))
#define OPTIMIZATIONS_OFF __pragma(optimize( "", off ))
#endif

#define BYTE_ALIGN(n) alignas(n)
