#pragma once

#include "Core/CoreDefines.h"

namespace Tinker
{
namespace Core
{
namespace Containers
{

#define CMP_FUNC(name) int name(const void* A, const void* B)
typedef CMP_FUNC(CompareFunc);

// Type-erased base class - avoid template compilation overhead
struct VectorBase
{
protected:
    uint8* m_data;
    uint32 m_size;
    uint32 m_capacity;

    void Reserve(uint32 numEles, uint32 eleSize);
    void Clear();
    void PushBackRaw(void* data, uint32 eleSize);
    uint32 Find(void* data, uint32 eleSize, CompareFunc Compare);
};

// NOTE: probably only works for POD types right now
template <typename T>
struct Vector : public VectorBase
{
    Vector()
    {
        m_data = nullptr;
        m_size = 0;
        m_capacity = 0;
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

    static CMP_FUNC(ShallowCompare)
    {
        return (int) (*((T*)A) == *((T*)B));
    }

    uint32 Find(const T& data)
    {
        return VectorBase::Find((void*)&data, sizeof(T), ShallowCompare);
    }
};


}
}
}
