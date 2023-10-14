#pragma once

#include "CoreDefines.h"

namespace Tk
{
namespace Core
{

struct Hash
{
    uint64 m_val;

    Hash() = delete;
    explicit Hash(uint64 val) : m_val(val) {}

};

TINKER_API Hash Hash64(const void* dataBuf, uint32 numBytes);

}
}
