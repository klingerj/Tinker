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
    
    ~HashMapBase()
    {
        CoreFree(m_data);
        m_data = nullptr;
        m_size = 0;
    }

private:
    uint32 ProbeFunc(uint32 index) const
    {
        return (index + 1) % m_size;
    }

protected:
    uint8* m_data;
    uint32 m_size;

    void Reserve(uint32 numEles, uint32 eleSize)
    {
        if (numEles > m_size)
        {
            void* newData = CoreMalloc(numEles * eleSize);
            memcpy(newData, m_data, m_size * eleSize);

            CoreFree(m_data); // free old data
            m_data = (uint8*)newData;

            // Init all other elements to invalid
            uint32 numRemainingEles = numEles - m_size;
            memset(m_data + m_size * eleSize, 255, numRemainingEles * eleSize);

            m_size = numEles;
        }
    }

    uint32 FindIndex(uint32 index, void* key, size_t dataPairSize, CompareKeyFunc Compare) const
    {
        uint32 currIndex = index;
        do
        {
            void* dataKey  = m_data + currIndex * dataPairSize;
            if (Compare(dataKey, key))
            {
                return currIndex;
            }

            currIndex = ProbeFunc(currIndex);
        }
        while (currIndex != index);

        return eInvalidIndex;
    }

    void* DataAtIndex(uint32 index, size_t dataPairSize, size_t dataValueOffset) const
    {
        TINKER_ASSERT(index < m_size);
        return m_data + index * dataPairSize + dataValueOffset;
    }

    uint32 Insert(uint32 index, void* key, void* value, CompareKeyFunc Compare, size_t dataPairSize, size_t dataValueOffset, size_t dataValueSize)
    {
        void* keyToInsert = m_data + index * dataPairSize;
        //TODO: don't hard code uint32 here
        uint32 invalid = eInvalidIndex;
        if (Compare(keyToInsert, &invalid) || Compare(keyToInsert, key)) // check if key is marked as invalid (unused) or matches the key
        {
            // found a slot
            memcpy(keyToInsert, key, dataValueOffset); // write key - assumes that offset is the same as key size
            memcpy((uint8*)keyToInsert + dataValueOffset, value, dataValueSize); // write value
        }
        else
        {
            // Do linear probing
        }

        return eInvalidIndex;
    }
};

template <typename T>
uint32 Hash(T val, uint32 dataSizeMax)
{
    return val % dataSizeMax;
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
        uint32 index = Hash(key, HashMapBase::eInvalidIndex - 1);
        return HashMapBase::FindIndex(index, &key, ePairSize, CompareKeyFunc_uint32);
    }

    const tVal& DataAtIndex(uint32 index) const
    {
        return *(tVal*)HashMapBase::DataAtIndex(index, ePairSize, ePairValOffset);
    }

    uint32 Insert(tKey key, tVal value)
    {
        TINKER_ASSERT(key != eInvalidIndex);
        uint32 index = Hash(key, m_size);
        return HashMapBase::Insert(index, &key, &value, CompareKeyFunc_uint32, ePairSize, ePairValOffset, ePairValSize);
    }
};

}
}
}
