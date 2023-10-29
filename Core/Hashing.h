#pragma once

#include "CoreDefines.h"

#define XXH_PRIVATE_API
#define XXH_INLINE_ALL
#define XXH_NO_STDLIB
#define XXH_NO_XXH3
#include "xxhash.h"

#include "constexpr-xxh3.h"

#define DEFAULT_STRING_HASH_SEED ((uint64)0x1234567887654321)

namespace Tk
{
namespace Core
{

struct Hash
{
    uint64 m_val;

    Hash() = delete;
    explicit constexpr Hash(uint64 val) : m_val(val) {}
};

}
}

#define HASH_64(buf) Tk::Core::Hash(constexpr_xxh3::XXH3_64bits_withSeed_const(buf, sizeof(buf), DEFAULT_STRING_HASH_SEED)) 
#define HASH_64_RUNTIME(buf, len) Tk::Core::Hash((uint64)(XXH64(buf, len, DEFAULT_STRING_HASH_SEED))) 
