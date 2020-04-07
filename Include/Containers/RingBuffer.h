#pragma once

#include "../System/SystemDefines.h"
#include "../Platform/PlatformAPI.h"

template <typename T, uint32 U>
// TODO: assert U >= 1?
class RingBuffer
{
private:
    uint32 _SIZE;
public:
    T* m_data = nullptr;
    BYTE_ALIGN(64) volatile uint32 m_head = UINT32_MAX;
    BYTE_ALIGN(64) volatile uint32 m_tail = UINT32_MAX;

    RingBuffer()
    {
        _SIZE = POW2_ROUNDUP(U);
    }

    ~RingBuffer()
    {
        if (m_data) delete m_data;
    }

    void Push(T ele)
    {
        if (!m_data) m_data = new T[_SIZE];
        m_data[Platform::AtomicIncrement32(&m_head) & (_SIZE - 1)] = ele;
    }

    T Pop()
    {
        T ToReturn = m_data[Platform::AtomicIncrement32(&m_tail) & (_SIZE - 1)];
        return ToReturn;
    }

    uint32 Capacity() const
    {
        return _SIZE;
    }

    uint32 Size() const
    {
        uint32 size = m_head - m_tail;
        return size;
    }
};
