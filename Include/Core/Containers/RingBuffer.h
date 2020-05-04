#pragma once

#include "../../Core/CoreDefines.h"
#include "../Include/PlatformGameAPI.h"

namespace Tinker
{
    namespace Containers
    {
        // Single Producer, Single Consumer wait-free queue
        template <typename T, uint32 size> // TODO: assert Size >= 1?
        class RingBuffer
        {
        private:
            uint32 _SIZE;
            uint32 _MASK;
        public:
            T* m_data = nullptr;
            BYTE_ALIGN(64) uint32 m_head = 0;
            BYTE_ALIGN(64) uint32 m_tail = 0;

            RingBuffer()
            {
                _SIZE = POW2_ROUNDUP(size);
                _MASK = _SIZE - 1;
                m_data = new T[_SIZE];
            }

            ~RingBuffer()
            {
                if (m_data) delete m_data;
            }

            uint32 Capacity() const
            {
                return _SIZE;
            }

            void Clear()
            {
                m_head = 0;
                m_tail = 0;
            }

            // Should only be called from the producer
            void Enqueue(T ele)
            {
                uint32 head = Platform::AtomicGet32(&m_head);
                m_data[head & _MASK] = ele;
                Platform::AtomicIncrement32(&m_head);
            }

            // Should only be called from the consumer
            void Dequeue(T* ele)
            {
                uint32 tail = Platform::AtomicGet32(&m_tail);
                *ele = m_data[tail & _MASK];
                Platform::AtomicIncrement32(&m_tail);
            }

            // Can be called from either producer or consumer
            uint32 Size()
            {
                const uint32 head = Platform::AtomicGet32(&m_head);
                const uint32 tail = Platform::AtomicGet32(&m_tail);
                return head - tail;
            }
        };
    }
}
