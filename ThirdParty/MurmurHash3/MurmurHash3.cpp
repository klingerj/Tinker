/*//-----------------------------------------------------------------------------
// MurmurHash3 was written by Austin Appleby, and is placed in the public
// domain. The author hereby disclaims copyright to this source code.

// Note - The x86 and x64 versions do _not_ produce the same results, as the
// algorithms are optimized for their respective platforms. You can still
// compile and run any of them on any platform, but your performance with the
// non-native version will be less than optimal.

#include "MurmurHash3.h"

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

// Microsoft Visual Studio

#if defined(_MSC_VER)

#define FORCE_INLINE	__forceinline

#include <stdlib.h>

//#define ROTL32(x,y)	_rotl(x,y)

// Other compilers

#else	// defined(_MSC_VER)

#define	FORCE_INLINE inline __attribute__((always_inline))

inline uint32_t rotl32 ( uint32_t x, int8_t r )
{
  return (x << r) | (x >> (32 - r));
}

#define	ROTL32(x,y)	rotl32(x,y)

#endif // !defined(_MSC_VER)

inline constexpr uint32_t rotl32(uint32_t x, int8_t r)
{
    return (x << r) | (x >> (32 - r));
}
#define	ROTL32(x,y)	rotl32(x,y)

//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here

static uint32_t getblock32 ( const uint32_t * p, int i )
{
  return p[i];
}

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

inline constexpr uint32_t fmix32 ( uint32_t h )
{
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;
  return h;
}

//-----------------------------------------------------------------------------

static constexpr uint32_t c1 = 0xcc9e2d51;
static constexpr uint32_t c2 = 0x1b873593;

static constexpr uint32_t Load4(const uint8_t* data)
{
    return (uint32_t(data[3]) << 24) | (uint32_t(data[2]) << 16) |
        (uint32_t(data[1]) << 8) | (uint32_t(data[0]) << 0);
}

static constexpr uint32_t MurmurLoopTail(int len, const uint8_t* data, uint32_t h)
{
    uint32_t hash = h;
    uint32_t k1 = 0;

    switch (len & 3)
    {
    case 3: k1 ^= data[2] << 16;
    case 2: k1 ^= data[1] << 8;
    case 1: k1 ^= data[0];
        k1 *= c1; k1 = ROTL32(k1, 15); k1 *= c2; hash ^= k1;
    }
    return hash;
}

static constexpr uint32_t MurmurBodyHash(uint32_t h, uint32_t data)
{
    uint32_t k1 = data;
    k1 *= c1;
    k1 = ROTL32(k1, 15);
    k1 *= c2;

    h ^= k1;
    h = ROTL32(h, 13);
    h = h * 5 + 0xe6546b64;
    return h;
}

static constexpr uint32_t MurmurLoopBody(int loopIters, int len, const uint8_t* data, uint32_t h)
{
    return (loopIters == 0 ?
        MurmurLoopTail(len, data, h) :
        MurmurLoopBody(loopIters - 1, len, data + 4, MurmurBodyHash(h, Load4(data))));
}

inline constexpr uint32_t MurmurHash3_x86_32(const char* key, int len, uint32_t seed)
{
    uint32_t h1 = seed;
    h1 = MurmurLoopBody(len / 4, len, (const uint8_t*)key, h1);
    h1 ^= len;
    h1 = fmix32(h1);
    return 0;
}

void MurmurHash3_x86_32(const void* key, int len,
    uint32_t seed, void* out)
{
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks = len / 4;

    uint32_t h1 = seed;

    //const uint32_t c1 = 0xcc9e2d51;
    //const uint32_t c2 = 0x1b873593;

    //----------
    // body

    const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);

    for (int i = -nblocks; i; i++)
    {
        uint32_t k1 = getblock32(blocks, i);

        k1 *= c1;
        k1 = ROTL32(k1, 15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ROTL32(h1, 13);
        h1 = h1 * 5 + 0xe6546b64;
    }

    //----------
    // tail

    const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);

    uint32_t k1 = 0;

    switch (len & 3)
    {
    case 3: k1 ^= tail[2] << 16;
    case 2: k1 ^= tail[1] << 8;
    case 1: k1 ^= tail[0];
        k1 *= c1; k1 = ROTL32(k1, 15); k1 *= c2; h1 ^= k1;
    };

    //----------
    // finalization

    h1 ^= len;

    h1 = fmix32(h1);

    *(uint32_t*)out = h1;
}

//-----------------------------------------------------------------------------
*/