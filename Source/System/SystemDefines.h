#pragma once

#include <stdint.h>
#include <math.h>
#include <cmath>

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

#define ISPOW2(x) x && ((x & (x - 1)) == 0)
template <typename T>
T POW2(T x)
{
    T i = 1;
    for (; x >> i; ++i)
    {
    }
    return 1 << i;
}
#define POW2_ROUNDUP(x) ISPOW2(x) ? x : POW2(x);

#define FLOAT_EQUAL(a, b) fabs(a - b) < FLT_EPSILON

#ifdef _WIN32
#define OPTIMIZATIONS_ON __pragma(optimize( "", on ))
#define OPTIMIZATIONS_OFF __pragma(optimize( "", off ))
#endif

#define BYTE_ALIGN(n) alignas(n)
