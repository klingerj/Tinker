#include "Core/Containers/Vector.h"
#include "Core/Utilities/Mem.h"

#include <string.h>

namespace Tinker
{
namespace Core
{
namespace Containers
{

VectorBase::~VectorBase()
{
    Utility::CoreFree(m_data);
    m_data = nullptr;
    m_size = 0;
    m_capacity = 0;
}

void VectorBase::Reserve(uint32 numEles, uint32 eleSize)
{
    if (numEles > m_capacity)
    {
        void* newData = Utility::CoreMalloc(numEles * eleSize);
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
        uint32 newCapacity = m_capacity + 1;
        void* newData = Utility::CoreMalloc(newCapacity * eleSize);
        memcpy(newData, m_data, m_capacity * eleSize);
        m_capacity = newCapacity;

        // Replace data
        Utility::CoreFree(m_data);
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

    return eInvalidIndex;
}

}
}
}
