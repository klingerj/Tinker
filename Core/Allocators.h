#pragma once

#include "CoreDefines.h"
#include "DataStructures/Vector.h"
#include "Mem.h"

namespace Tk
{
  namespace Core
  {
    struct LinearAllocator
    {
      uint8* m_ownedMemPtr = nullptr;
      size_t m_capacity = 0;
      size_t m_nextAllocOffset = 0;

      LinearAllocator() = default;

      ~LinearAllocator()
      {
        ExplicitFree();
      }

      void ExplicitFree()
      {
        if (m_ownedMemPtr)
        {
          Tk::Core::CoreFreeAligned(m_ownedMemPtr);
          m_ownedMemPtr = nullptr;
        }
        m_nextAllocOffset = 0;
        m_capacity = 0;
      }

      void Init(size_t capacity, uint32 alignment)
      {
        TINKER_ASSERT(capacity > 0);
        TINKER_ASSERT(alignment > 0);
        m_capacity = capacity;
        m_ownedMemPtr =
          static_cast<uint8*>(Tk::Core::CoreMallocAligned(m_capacity, alignment));
        m_nextAllocOffset = 0;
      }

      void Init(void* existingBuffer, uint32 existingBufferSize)
      {
        TINKER_ASSERT(existingBuffer);
        TINKER_ASSERT(existingBufferSize > 0);
        m_capacity = existingBufferSize;
        m_ownedMemPtr = static_cast<uint8*>(existingBuffer);
        m_nextAllocOffset = 0;
      }

      uint8* Alloc(size_t allocSizeReq, uint32 alignment)
      {
        TINKER_ASSERT(ISPOW2(alignment));

        size_t memPtrAsNum = reinterpret_cast<size_t>(static_cast<uint8*>(m_ownedMemPtr)
                                                      + m_nextAllocOffset);

        // If the current memory pointer is not aligned, add the alignment as offset
        const size_t alignmentBits = LOG2(alignment);
        size_t alignedPtrAsNum = (memPtrAsNum >> alignmentBits) << alignmentBits;
        alignedPtrAsNum +=
          alignment * static_cast<size_t>(alignedPtrAsNum != memPtrAsNum);

        // Check that there is room for this allocation
        const size_t allocSizeActual = alignedPtrAsNum - memPtrAsNum + allocSizeReq;
        const size_t remainingBytes = m_capacity - m_nextAllocOffset;
        if (allocSizeActual > remainingBytes)
        {
          return nullptr; // fail, no assert
        }

        m_nextAllocOffset += allocSizeActual;
        return reinterpret_cast<uint8*>(alignedPtrAsNum);
      }

      void ResetState()
      {
        m_nextAllocOffset = 0;
      }

      void* Data() const
      {
        return m_ownedMemPtr;
      }

      size_t Capacity() const
      {
        return m_capacity;
      }

      size_t Size() const
      {
        return m_nextAllocOffset;
      }
    };

    struct StackAllocator : protected LinearAllocator
    {
      StackAllocator()
        : LinearAllocator()
      {
      }

      ~StackAllocator() = default;

    protected:
      Vector<size_t> m_allocOffsetStack;

      void DoPushAllocation(size_t prevAllocOffset)
      {
        m_allocOffsetStack.PushBackRaw(prevAllocOffset);
      }

      void DoPopAllocation()
      {
        TINKER_ASSERT(m_allocOffsetStack.Size() > 0);
        const size_t prevAllocOffset = m_allocOffsetStack[m_allocOffsetStack.Size() - 1];
        m_allocOffsetStack.PopBack();

        // Restore internal allocator state.
        // TODO turn this into nicer function call
        m_nextAllocOffset = prevAllocOffset;
      }

    public:
      void Init(size_t capacity, uint32 alignment)
      {
        LinearAllocator::Init(capacity, alignment);

        // TODO: refine this initial alloc value
        m_allocOffsetStack.Reserve(128);
      }

      uint8* Alloc(size_t allocSizeReq, uint32 alignment)
      {
        DoPushAllocation(m_nextAllocOffset);
        return LinearAllocator::Alloc(allocSizeReq, alignment);
      }

      void Free()
      {
        DoPopAllocation();
      }
    };

    // TODO: make this a fixed vector
    extern Vector<StackAllocator> g_PerThreadTempAllocators;
    TINKER_API void InitPerThreadAllocators(uint32 numThreadsIncludingMain);
    TINKER_API StackAllocator& GetThreadTempAllocator();

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
    struct PoolAllocator
    {
      uint32 m_numAllocdElements = 0;
      uint32 m_maxPoolElements = 0;
      uint32 m_freeListHead = 0;

      template <typename U>
      using PoolElement = struct pool_element<U>;
      PoolElement<T>* m_pool = nullptr;
      uint32 m_elementSizeInBytes = sizeof(PoolElement<T>);

      PoolAllocator()
      {
        TINKER_ASSERT(ISPOW2(Alignment));
        if (NumElements > 0)
        {
          Init(NumElements, Alignment);
        }
        else
        {
          // User must specify alloc'd memory with Init()
        }
      }

      ~PoolAllocator()
      {
        ExplicitFree();
      }

      void ExplicitFree()
      {
        if (m_pool)
        {
          Tk::Core::CoreFreeAligned(m_pool);
          m_pool = nullptr;
          m_freeListHead = 0;
          m_maxPoolElements = 0;
        }
      }

      inline T* PtrFromHandle(uint32 handle)
      {
        return &m_pool[handle].m_data;
      }

      void Init(uint32 maxPoolElements, size_t alignment)
      {
        TINKER_ASSERT(m_maxPoolElements == 0);
        // Only call Init() if you did not provide the number of elements as a template at
        // compile-time.

        TINKER_ASSERT(maxPoolElements > 0 && maxPoolElements < TINKER_INVALID_HANDLE);
        m_maxPoolElements = maxPoolElements;

        m_pool = (PoolElement<T>*)Tk::Core::CoreMallocAligned(
          m_maxPoolElements * m_elementSizeInBytes, alignment);

        // Init free list
        for (uint32 uiEle = 0; uiEle < m_maxPoolElements - 1; ++uiEle)
        {
          m_pool[uiEle].m_nextFreeEleIdx = uiEle + 1; // point to next element
        }
        m_pool[m_maxPoolElements - 1].m_nextFreeEleIdx = TINKER_INVALID_HANDLE;
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
  } //namespace Core
} //namespace Tk
