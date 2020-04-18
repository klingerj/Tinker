#pragma once

#include "../PlatformGameAPI.h"

namespace Tinker
{
    namespace Memory
    {
        template <size_t Size, size_t Alignment = 1>
        class LinearAllocator
        {
        private:
            uint8* m_ownedMemPtr = nullptr;
            size_t m_nextAllocOffset = 0;

        public:
            LinearAllocator()
            {
                TINKER_ASSERT(Size > 0);
                TINKER_ASSERT(ISPOW2(Alignment));
                m_ownedMemPtr = (uint8*)Platform::AllocAligned(Size, Alignment);
            }

            ~LinearAllocator()
            {
                Free();
            }

            void Free()
            {
                m_nextAllocOffset = 0;
                if (m_ownedMemPtr) Platform::FreeAligned(m_ownedMemPtr);
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
                if (allocSize > Size - m_nextAllocOffset) return nullptr;

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
