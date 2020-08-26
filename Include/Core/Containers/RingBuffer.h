#pragma once

#include "../../Core/CoreDefines.h"
#include "../../PlatformGameAPI.h"
#include <atomic>
#include <xmmintrin.h>

namespace Tinker
{
    namespace Containers
    {
        // Single Producer, Single Consumer wait-free queue
        template <typename T, uint32 size> // TODO: assert Size >= 1? or that it is a power of 2?
        class RingBuffer
        {
        private:
            uint32 _SIZE;
            uint32 _MASK;
        public:
            T* m_data = nullptr;
            BYTE_ALIGN(64) volatile std::atomic<uint32> m_head = 0;
            BYTE_ALIGN(64) volatile std::atomic<uint32> m_tail = 0;

            RingBuffer()
            {
                _SIZE = POW2_ROUNDUP(size);
                _MASK = _SIZE - 1;
                m_data = (T*)Platform::AllocAligned(_SIZE * sizeof(T), 64);
            }

            ~RingBuffer()
            {
                if (m_data) Platform::FreeAligned(m_data);
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
                uint32 head = std::atomic_load_explicit(&m_head, std::memory_order_acquire);
                m_data[head & _MASK] = ele;
                
                std::atomic_thread_fence(std::memory_order_release);
                _mm_sfence();

                atomic_fetch_add(&m_head, 1);
            }

            // Should only be called from the consumer
            void Dequeue(T* ele)
            {
                uint32 tail = std::atomic_load_explicit(&m_tail, std::memory_order_acquire);
                *ele = m_data[tail & _MASK];

                std::atomic_thread_fence(std::memory_order_acquire);

                atomic_fetch_add(&m_tail, 1);
            }

            // Can be called from either producer or consumer
            uint32 Size()
            {
                const uint32 head = atomic_load_explicit(&m_head, std::memory_order_acquire);
                const uint32 tail = atomic_load_explicit(&m_tail, std::memory_order_acquire);
                return head - tail;
            }
        };
    }
}
