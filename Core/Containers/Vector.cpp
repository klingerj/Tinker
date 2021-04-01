#include "Core/Containers/Vector.h"

#include <string.h>

namespace Tinker
{
namespace Core
{
namespace Containers
{

void VectorBase::Reserve(uint32 numEles, uint32 eleSize)
{
    if (numEles > m_capacity)
    {
        void* newData = malloc(numEles * eleSize);
        memcpy(newData, m_data, m_size * eleSize);
        m_data = (uint8*)newData;
        m_capacity = numEles;
    }
}

void VectorBase::Clear()
{
    m_size = 0;
}

void VectorBase::PushBackRaw(void* data, uint32 eleSize)
{
    if (m_size == m_capacity)
    {
        // Alloc new data
        uint32 newCapacity = m_capacity  + 1;
        void* newData = malloc(newCapacity * eleSize);
        memcpy(newData, m_data, m_capacity * eleSize);
        m_capacity = newCapacity;

        // Replace data
        free(m_data);
        m_data = (uint8*)newData;
    }

    if (m_size < m_capacity)
    {
        memcpy(m_data + m_size * eleSize, data, eleSize);
        ++m_size;
    }
}

uint32 VectorBase::Find(void* data, uint32 eleSize, CompareFunc Compare) const
{
    for (uint32 i = 0; i < m_size; ++i)
    {
        if (Compare(data, m_data + i * eleSize))
        {
            return i;
        }
    }

    return 0xffffffff;
}

}
}
}
