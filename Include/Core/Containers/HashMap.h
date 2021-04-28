#pragma once

#include "Core/CoreDefines.h"

// Open addressing hashmap
struct HashMapBase
{
    enum { eInvalidIndex = MAX_UINT32 };
    
    ~HashMapBase()
    {
        Utility::CoreFree(m_data);
        m_data = nullptr;
        m_size = 0;
        m_capacity = 0;
    }

private:
    uint32 ProbeFunc(uint32 index)
    {
        return (index + 1) % m_size;
    }

protected:
    uint8* m_data;
    uint32 m_size;
    uint32 m_capacity;

    uint32 FindIndex(uint32 index, void* key, size_t dataPairSize, KeyCompareFunc Compare) const
    {
        uint32 currIndex = index;
        do
        {
            void* dataKey  = m_data + i * (dataPairSize);
            if (Compare(dataKey, key))
            {
                return currIndex; //dataKey + valueOffset;
            }

            currIndex = ProbFunc(currIndex);
        }
        while (currIndex != index)

        return eInvalidIndex;
    }

    void* DataAtIndex(uint32 index, size_t dataPairSize, size_t dataValueOffset) const
    {
        TINKER_ASSERT(index < m_size);
        return m_data + index * dataPairSize + dataValueOffset;
    }

    uint32 Insert(uint32 index, void* key, void* value, KeyCompareFuncInvalid Compare, size_t dataPairSize, size_t dataValueOffset, size_t dataValueSize)
    {
        void* dataToInsert = m_data + index * dataPairSize;
        if (Compare(dataToInsert)) // check if key is marked as invalid (unused)
        {
            // found a slot
            memcpy(dataToInsert, key, dataValueOffset); // write key
            memcpy(dataToInsert + dataValueOffset, value, dataValueSize); // write value
        }
        else
        {
            // Do linear probing
        }

        return 0;
    }
};

template <typename T>
uint32 Hash(T val)
{
    return 0;
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
        ePairValSize = size(Pair::value),
        ePairValOffset = offsetof(Pair::value)
    };

public:
    uint32 FindIndex(tKey key) const
    {
        uint32 index = Hash(key);
        return HashMapBase::Find(index, &key, ePairSize, KeyCompareFunc Compare);
    }

    tVal DataAtIndex(uint32 index) const
    {
        return *(tVal*)HashMapBase::DataAtIndex(index, ePairSize, ePairValOffset);
    }

    uint32 Insert(tKey key, tVal value)
    {
        TINKER_ASSERT(key != eInvalidIndex);
        uint32 index = Hash(key);
        return HashMapBase::Insert(index, key, value, ePairSize, KeyCompareFuncInvalid Compare, ePairValOffset, ePairValSize);
    }
};



// TODO: Open addressing hashmap with cache and manual flushing


// Benchmarks to do:
//- insert many in order, and also random
//- find many in order, and also random
