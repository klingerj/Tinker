#pragma once

#include "Core/CoreDefines.h"
#include "Core/Mem.h"

#include <string.h>

#define CMP_KEY_FUNC(name) bool name(const void* A, const void* B)
typedef CMP_KEY_FUNC(CompareKeyFunc);

// Define custom compare funcs like so:
inline CMP_KEY_FUNC(CompareKeyFunc_uint32)
{ 
    return *(uint32*)A == *(uint32*)B;
}

namespace Tinker
{
namespace Core
{
namespace Containers
{

// Open addressing hashmap
struct HashMapBase
{
    // TODO: assuming uint32 here
    enum : uint32 { eInvalidIndex = MAX_UINT32 };
    
    ~HashMapBase();

private:
    uint32 ProbeFunc(uint32 index) const
    {
        return (index + 1) % m_size;
    }

protected:
    uint8* m_data;
    uint32 m_size;

    void Reserve(uint32 numEles, uint32 eleSize);
    uint32 FindIndex(uint32 index, void* key, size_t dataPairSize, CompareKeyFunc Compare) const;
    void* DataAtIndex(uint32 index, size_t dataPairSize, size_t dataValueOffset) const;
    uint32 Insert(uint32 index, void* key, void* value, CompareKeyFunc Compare, size_t dataPairSize, size_t dataValueOffset, size_t dataValueSize);
};


// Good hash function discovered here: https://github.com/skeeto/hash-prospector 
inline uint32 lowbias32(uint32 x)
{
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

template <typename T>
inline uint32 Hash(T val, uint32 dataSizeMax)
{
    return val % dataSizeMax;
}

template <>
inline uint32 Hash<uint32>(uint32 val, uint32 dataSizeMax)
{
    return lowbias32(val) % dataSizeMax;
}

template <typename tKey, typename tVal>
struct HashMap : public HashMapBase
{
private:
    struct Pair
    {
        tKey key;
        tVal value;
    };

    enum
    {
        ePairSize = sizeof(Pair),
        ePairValSize = sizeof(Pair::value),
        ePairValOffset = sizeof(Pair) - sizeof(Pair::value), // TODO: struct alignment issues?
    };

public:
    HashMap() : HashMapBase()
    {
        m_data = nullptr;
        m_size = 0;
    }
    
    void Reserve(uint32 numEles)
    {
        HashMapBase::Reserve(numEles, ePairSize);
    }

    uint32 FindIndex(tKey key) const
    {
        uint32 index = Hash(key, m_size);
        return HashMapBase::FindIndex(index, &key, ePairSize, CompareKeyFunc_uint32);
    }

    const tVal& DataAtIndex(uint32 index) const
    {
        return *(tVal*)HashMapBase::DataAtIndex(index, ePairSize, ePairValOffset);
    }

    uint32 Insert(tKey key, tVal value)
    {
        uint32 index = Hash(key, m_size);
        return HashMapBase::Insert(index, &key, &value, CompareKeyFunc_uint32, ePairSize, ePairValOffset, ePairValSize);
    }
};

}
}
}
