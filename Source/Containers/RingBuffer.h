#pragma once

#include "../System/CoreDefines.h"
#include "../Platform/PlatformAPI.h"

template <typename T, uint32 U>
// TODO: assert U >= 1?
class RingBuffer
{
public:
    BYTE_ALIGN(64) T* m_data;
    BYTE_ALIGN(4) volatile uint32 head = UINT32_MAX;
    BYTE_ALIGN(4) volatile uint32 tail = UINT32_MAX;

    RingBuffer()
    {
        m_data = new T[U];
    }

    ~RingBuffer()
    {
        delete m_data;
    }

    void Push(T ele)
    {
        m_data[AtomicIncrement32(&head) & (U - 1)] = ele;
    }

    T Pop()
    {
        return m_data[AtomicIncrement32(&tail) & (U - 1)];
    }

    uint32 Capacity() const
    {
        return U;
    }

    uint32 Size() const
    {
        return (head & (U - 1)) - (tail & (U - 1));
    }
};
