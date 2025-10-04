#pragma once

#include "CoreDefines.h"

#define XXH_PRIVATE_API
#define XXH_INLINE_ALL
#define XXH_NO_STDLIB
#define XXH_NO_XXH3
#include "ThirdParty/constexpr-xxh3/constexpr-xxh3.h"
#include "ThirdParty/xxHash-0.8.2/xxhash.h"

#define DEFAULT_STRING_HASH_SEED ((uint64)0x12'34'56'78'87'65'43'21)

namespace Tk
{
  namespace Core
  {
    struct Hash
    {
      uint64 m_val;

      Hash() = delete;

      explicit constexpr Hash(uint64 val)
        : m_val(val)
      {
      }
    };
  } //namespace Core
} //namespace Tk

#define HASH_64(buf)                                                                     \
  Tk::Core::Hash(constexpr_xxh3::XXH3_64bits_withSeed_const(buf, sizeof(buf),            \
                                                            DEFAULT_STRING_HASH_SEED))
#define HASH_64_RUNTIME(buf, len)                                                        \
  Tk::Core::Hash((uint64)(XXH64(buf, len, DEFAULT_STRING_HASH_SEED)))
