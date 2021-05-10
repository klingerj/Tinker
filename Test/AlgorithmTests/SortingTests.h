#include "Core/Sorting.h"
#include "TinkerTest.h"

// TODO: have own rng
#include <random>
#include <algorithm>
#include <array>
void Test_SortingIntegers()
{
    const uint32 numUints = 65536;
    uint32 uintArr[numUints] = {};
    uint32 uintArr_MergeSorted[numUints] = {};
    std::array<uint32, numUints> uintArr_StdSorted = {};
    for (uint32 i = 0; i < numUints; ++i)
    {
        uintArr[i] = rand() % 16;
        uintArr_StdSorted[i] = uintArr[i];
    }

    memcpy(&uintArr_MergeSorted[0], &uintArr[0], sizeof(uint32) * numUints);

    Core::MergeSort(&uintArr_MergeSorted[0], numUints, CompareLessThan_uint32);
    std::sort(uintArr_StdSorted.begin(), uintArr_StdSorted.end());
    
    for (uint32 i = 0; i < numUints; ++i)
    {
        TINKER_ASSERT(uintArr_MergeSorted[i] == uintArr_StdSorted[i]);
    }
}

