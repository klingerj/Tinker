#pragma once

#include "../Include/System/Allocators.h"
#include "../TinkerTest.h"

using namespace Tinker;
using namespace Memory;

// Linear allocator
void Test_Linear_NoAlignment()
{
    LinearAllocator<1024> allocator;
    for (uint16 i = 0; i < 1024; ++i)
    {
        uint8* ptr = (uint8*)allocator.Alloc(1, 1);
        TINKER_TEST_ASSERT(ptr);
    }
    uint8* ptr = (uint8*)allocator.Alloc(1, 1);
    TINKER_TEST_ASSERT(!ptr);
}

void Test_Linear_Alignment()
{
    LinearAllocator<1024, 2> allocator;
    for (uint16 i = 0; i < 1024 / 16; i += 16)
    {
        uint8* ptr = (uint8*)allocator.Alloc(1, 16);
        
        size_t ptrAsNum = (size_t)ptr;
        TINKER_TEST_ASSERT(((ptrAsNum >> 4) << 4) == ptrAsNum);
    }
}

void Test_Linear_NoAlignment_WithDealloc()
{
    LinearAllocator<1024> allocator;
    for (uint16 i = 0; i < 1024; ++i)
    {
        uint8* ptr = (uint8*)allocator.Alloc(1, 1);
        TINKER_TEST_ASSERT(ptr);
    }
    uint8* ptr = (uint8*)allocator.Alloc(1, 1);
    TINKER_TEST_ASSERT(!ptr);

    allocator.Dealloc();

    for (uint16 i = 0; i < 1024; ++i)
    {
        uint8* ptr2 = (uint8*)allocator.Alloc(1, 1);
        TINKER_TEST_ASSERT(ptr2);
    }
    ptr = (uint8*)allocator.Alloc(1, 1);
    TINKER_TEST_ASSERT(!ptr);
}
