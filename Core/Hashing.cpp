#pragma once

#include "Hashing.h"

#include "xxhash.h"

#define DEFAULT_STRING_HASH_SEED ((uint64)0x87654321)

namespace Tk
{
namespace Core
{

Hash Hash64(const void* dataBuf, uint32 numBytes)
{
    XXH64_hash_t hash = XXH64(dataBuf, numBytes, DEFAULT_STRING_HASH_SEED);
    return Hash((uint64)hash);
}

}
}
