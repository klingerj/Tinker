#pragma once

#include "../PlatformGameAPI.h"

namespace Tinker
{
    namespace Memory
    {
        template <size_t Size = 0, size_t Alignment = 1>
        class LinearAllocator
        {
        private:
            size_t m_size;
            uint8* m_ownedMemPtr = nullptr;
            size_t m_nextAllocOffset = 0;

        public:
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
                if (m_ownedMemPtr) Platform::FreeAligned(m_ownedMemPtr);
            }

            void Init(size_t size, size_t alignment)
            {
                TINKER_ASSERT(size > 0);
                m_size = size;
                m_ownedMemPtr = (uint8*)Platform::AllocAligned(m_size, Alignment);
            }

            uint8* Alloc(size_t size, size_t alignment)
            {
                TINKER_ASSERT(ISPOW2(alignment));

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
    }
}
