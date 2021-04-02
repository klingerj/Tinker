#pragma once

#include "Core/CoreDefines.h"

namespace Tinker
{
namespace Core
{
namespace Containers
{

#define CMP_FUNC(name) bool name(const void* A, const void* B)
typedef CMP_FUNC(CompareFunc);

// Type-erased base class - avoid template compilation overhead
struct VectorBase
{
    enum : uint32 { eInvalidIndex = MAX_UINT32 };

    ~VectorBase();

protected:
    uint8* m_data;
    uint32 m_size;
    uint32 m_capacity;

    void Reserve(uint32 numEles, uint32 eleSize);
    void Clear();
    void PushBackRaw(void* data, uint32 eleSize);
    uint32 Find(void* data, uint32 eleSize, CompareFunc Compare) const;
};

// NOTE: probably only works for POD types right now
// Actual templated vector class
template <typename T>
struct Vector : public VectorBase
{
    Vector() : VectorBase()
    {
        m_data = nullptr;
        m_size = 0;
        m_capacity = 0;
    }
    const uint8* Data() const
    {
        return m_data;
    }
    uint32 Size() const
    {
        return m_size;
    }
    uint32 Capacity() const
    {
        return m_capacity;
    }

    void Reserve(uint32 numEles)
    {
        VectorBase::Reserve(numEles, sizeof(T));
    }

    void Clear()
    {
        VectorBase::Clear();
    }

    void PushBackRaw(const T& data)
    {
        VectorBase::PushBackRaw((void*)&data, sizeof(T));
    }

    static CMP_FUNC(DefaultEqualsCompare)
    {
        return *((T*)A) == *((T*)B);
    }

    uint32 Find(const T& data) const
    {
        return VectorBase::Find((void*)&data, sizeof(T), DefaultEqualsCompare);
    }

    const T& operator[](uint32 index) const
    {
        TINKER_ASSERT(index < m_size);
        return ((T*)(m_data))[index];
    }

    T& operator[](uint32 index)
    {
        TINKER_ASSERT(index < m_size);
        return ((T*)(m_data))[index];
    }
};


}
}
}
