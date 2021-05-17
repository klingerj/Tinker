#include "Core/Containers/HashMap.h"
#include "Core/Mem.h"

#include <string.h>

namespace Tk
{
namespace Core
{
namespace Containers
{

HashMapBase::~HashMapBase()
{
    CoreFree(m_data);
    m_data = nullptr;
    m_size = 0;
}

void HashMapBase::Reserve(uint32 numEles, uint32 eleSize)
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

uint32 HashMapBase::FindIndex(uint32 index, void* key, size_t dataPairSize, CompareKeyFunc Compare) const
{
    uint32 invalid = eInvalidIndex;
    if (Compare(key, &invalid)) { return eInvalidIndex; }

    uint32 currIndex = index;
    do
    {
        void* dataKey  = m_data + currIndex * dataPairSize;
        if (Compare(dataKey, key))
        {
            return currIndex;
        }

        currIndex = ProbeFunc(currIndex);
    } while (currIndex != index);

    return eInvalidIndex;
}

void* HashMapBase::DataAtIndex(uint32 index, size_t dataPairSize, size_t dataValueOffset) const
{
    TINKER_ASSERT(index < m_size);
    return m_data + index * dataPairSize + dataValueOffset;
}

uint32 HashMapBase::Insert(uint32 index, void* key, void* value, CompareKeyFunc Compare, size_t dataPairSize, size_t dataValueOffset, size_t dataValueSize)
{
    //TODO: don't hard code uint32 here
    uint32 invalid = eInvalidIndex;
    if (Compare(key, &invalid)) { return eInvalidIndex; }

    uint32 currIndex = index;
    do
    {
        void* keyToInsertAt = m_data + currIndex * dataPairSize;

        // check if key is marked as invalid (unused) or matches the input key
        if (Compare(keyToInsertAt, &invalid) || Compare(keyToInsertAt, key))
        {
            // found a slot
            memcpy(keyToInsertAt, key, dataValueOffset); // write key - assumes that offset is the same as key size
            memcpy((uint8*)keyToInsertAt + dataValueOffset, value, dataValueSize); // write value
            return currIndex;
        }
        else
        {
            currIndex = ProbeFunc(currIndex);
        }
    } while (currIndex != index);

    return eInvalidIndex;
}

}
}
}
