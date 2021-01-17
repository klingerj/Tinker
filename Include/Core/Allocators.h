#pragma once

#include "../PlatformGameAPI.h"

namespace Tinker
{
    namespace Memory
    {
        template <size_t Size = 0, uint32 Alignment = 1>
        class LinearAllocator
        {
        public:
            size_t m_size;
            uint8* m_ownedMemPtr = nullptr;
            size_t m_nextAllocOffset = 0;

            LinearAllocator()
            {
                TINKER_ASSERT(ISPOW2(Alignment));
                m_size = Size;
                if (m_size > 0)
                {
                    m_ownedMemPtr = (uint8*)Platform::AllocAligned(Size, Alignment);
                }
                else
                {
                    // User must specify alloc'd memory with Init()
                }
            }

            ~LinearAllocator()
            {
                Free();
            }

            void Free()
            {
                if (m_ownedMemPtr)
                {
                    Platform::FreeAligned(m_ownedMemPtr);
                    m_ownedMemPtr = nullptr;
                }
                m_nextAllocOffset = 0;
                m_size = 0;
            }

            void Init(size_t size, uint32 alignment)
            {
                TINKER_ASSERT(size > 0);
                m_size = size;
                m_ownedMemPtr = (uint8*)Platform::AllocAligned(m_size, alignment);
            }

            uint8* Alloc(size_t size, size_t alignment)
            {
                TINKER_ASSERT(ISPOW2(alignment));

                if (!((m_size - m_nextAllocOffset) >= size))
                {
                    // Not enough space - allocation fails, but don't assert
                    return nullptr;
                }

                size_t memPtrAsNum = (size_t)((uint8*)m_ownedMemPtr + m_nextAllocOffset);

                // If the current memory pointer is not aligned, add the alignment as offset
                size_t alignmentBits = LOG2(alignment);
                size_t alignedPtrAsNum = (memPtrAsNum >> alignmentBits) << alignmentBits;
                if (alignedPtrAsNum != memPtrAsNum)alignedPtrAsNum += alignment;
                
                // Check that the allocator has room for this allocation
                size_t allocSize = alignedPtrAsNum - memPtrAsNum + size;
                if (allocSize > m_size - m_nextAllocOffset) return nullptr;

                // Return new pointer
                uint8* newAllocPtr = (uint8*)alignedPtrAsNum;
                m_nextAllocOffset += allocSize;
                return newAllocPtr;
            }

            void Dealloc()
            {
                m_nextAllocOffset = 0;
            }
        };

        template <typename T>
        struct pool_element
        {
            union
            {
                struct
                {
                    T m_data;
                };
                struct
                {
                    uint32 m_nextFreeEleIdx;
                };
            };
        };

        template <typename T, uint32 NumElements = 0, uint32 Alignment = 1>
        class PoolAllocator
        {
        private:
            uint32 m_numAllocdElements = 0;
            uint32 m_maxPoolElements = 0;
            uint32 m_freeListHead = 0;

            template <typename U>
            using PoolElement = struct pool_element<U>;
            PoolElement<T>* m_pool = nullptr;
            uint32 m_elementSizeInBytes = sizeof(PoolElement<T>);

            void InitFreeListPtrs()
            {
                for (uint32 uiEle = 0; uiEle < m_maxPoolElements - 1; ++uiEle)
                {
                    m_pool[uiEle].m_nextFreeEleIdx = uiEle + 1; // point to next element
                }
                m_pool[m_maxPoolElements - 1].m_nextFreeEleIdx = TINKER_INVALID_HANDLE;
            }

        public:
            PoolAllocator()
            {
                TINKER_ASSERT(ISPOW2(Alignment));
                m_maxPoolElements = NumElements;
                if (m_maxPoolElements > 0)
                {
                    m_pool = (PoolElement<T>*)Platform::AllocAligned(m_maxPoolElements * m_elementSizeInBytes, Alignment);
                    InitFreeListPtrs();
                }
                else
                {
                    // User must specify alloc'd memory with Init()
                }
            }

            ~PoolAllocator()
            {
                if (m_pool) Platform::FreeAligned(m_pool);
            }

            inline T* PtrFromHandle(uint32 handle)
            {
                return &m_pool[handle].m_data;
            }

            void Init(uint32 maxPoolElements, size_t alignment)
            {
                TINKER_ASSERT(m_maxPoolElements == 0);
                // Only call Init() if you did not provide the number of elements as a template at compile-time.

                TINKER_ASSERT(maxPoolElements > 0 && maxPoolElements < TINKER_INVALID_HANDLE);
                m_maxPoolElements = maxPoolElements;
                m_pool = (PoolElement<T>*)Platform::AllocAligned(m_maxPoolElements * m_elementSizeInBytes, alignment);
                InitFreeListPtrs();
            }

            uint32 Alloc()
            {
                if (m_freeListHead == TINKER_INVALID_HANDLE)
                {
                    // Pool is full - resize?
                    TINKER_ASSERT(0);
                    return TINKER_INVALID_HANDLE;
                }
                else
                {
                    uint32 newEle = m_freeListHead;
                    m_freeListHead = m_pool[m_freeListHead].m_nextFreeEleIdx;
                    ++m_numAllocdElements;

                    if (m_numAllocdElements == m_maxPoolElements)
                    {
                        m_freeListHead = TINKER_INVALID_HANDLE;
                    }

                    return newEle;
                }
            }

            void Dealloc(uint32 handle)
            {
                TINKER_ASSERT(m_numAllocdElements > 0);
                TINKER_ASSERT(handle < m_maxPoolElements);
                m_pool[handle].m_nextFreeEleIdx = m_freeListHead;
                m_freeListHead = handle;
                --m_numAllocdElements;
            }
        };
    }
}
