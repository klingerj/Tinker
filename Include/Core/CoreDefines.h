#pragma once

#include <stdint.h>
#include <math.h>
#include <cmath>

#ifdef ASSERTS_ENABLE
#if ASSERTS_ENABLE

#endif
#endif

#ifdef ASSERTS_ENABLE
#include <assert.h>
#define TINKER_ASSERT(cond) assert((cond))
#else
#define TINKER_ASSERT(cond) 
#endif

// C++ only!
#define RESTRICT __restrict
#define _STRINGIFY(s) #s
#define STRINGIFY(s) _STRINGIFY(s)

typedef uint8_t  uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int8_t   int8;
typedef int16_t  int16;
typedef int32_t  int32;
typedef int64_t  int64;

#define MAX_UINT64 0xFFFFFFFFFFFFFFFF
#define MAX_UINT32 0xFFFFFFFF
#define MAX_UINT16 0xFFFF

#define TINKER_INVALID_HANDLE MAX_UINT32

template <typename T>
inline T max(const T& a, const T& b)
{
    return a > b ? a : b;
}

template <typename T>
inline T min(const T& a, const T& b)
{
    return a < b ? a : b;
}
#define CLAMP(a, b, c) min(max(a, b), c)

template <typename T>
inline bool ISPOW2(const T& x)
{
    return x && ((x & (x - 1)) == 0);
}

template <typename T>
inline T LOG2(T x)
{
    T i = 1;
    for (; x >> i; ++i) {}
    return i;
}

template <typename T>
inline T POW2(T x)
{
    return 1 << LOG2(x);
}

template <typename T>
inline T POW2_ROUNDUP(const T& x)
{
    return ISPOW2(x) ? x : POW2(x);
}

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

#define CACHE_LINE 64

// Quick simple memory allocation and leak tracking
// NOTE: placement new doesn't compile nicely with this.
//#define DISABLE_MEM_TRACKING

#if !defined(DISABLE_MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
#define MEM_TRACKING
#endif

#if defined(MEM_TRACKING) && defined(_WIN32) && defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif
