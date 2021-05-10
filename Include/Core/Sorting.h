#pragma once

#include "Core/CoreDefines.h"
#include "Core/Mem.h"

namespace Tinker
{
namespace Core
{

inline bool uint32_cmp_lt(const uint32& a, const uint32& b) { return a < b; }

template <typename T, typename CompareLessThan>
inline void Merge(T* left, uint32 numElesLeft, T* right, uint32 numElesRight, CompareLessThan Compare)
{
    uint32 totalEles = numElesLeft + numElesRight;
    // TODO: get rid of this temp malloc'd mem
    T* tmpList = (T*)CoreMalloc(totalEles * sizeof(T));
    memcpy(tmpList, left, sizeof(T) * numElesLeft);
    memcpy(tmpList + numElesLeft, right, sizeof(T) * numElesRight);

    uint32 i = 0, j = 0;
    while (1)
    {
        if (i + j == totalEles) break;
        // TODO: probably add other conditions, e.g. i == numElesLeft or j == numElesRight
        // also not sure that this condition is correct

        // Assuming that left and right are actually contiguous
        T* dstList = left + i + j;
        
        // Copy over left's next ele if i is not done counting AND either j is done, or ith < jth ele
        if ((Compare(tmpList[i], tmpList[numElesLeft + j]) || j >= numElesRight) && i < numElesLeft)
        {
            memcpy(dstList, &tmpList[i], sizeof(T));
            ++i;
        }
        else
        {
            memcpy(dstList, &tmpList[numElesLeft + j], sizeof(T));
            ++j;
        }
    }
    CoreFree(tmpList);
}

template <typename T, typename CompareLessThan>
inline void MergeSort(T* data, uint32 numEles, CompareLessThan Compare)
{
    if (numEles == 1) return;

    uint32 midpoint = numEles / 2;

    T* left = data;
    T* right = data + midpoint;
    uint32 leftSize = midpoint;
    uint32 rightSize = numEles - midpoint;

    // Sort each half
    MergeSort(left, leftSize, Compare);
    MergeSort(right, rightSize, Compare);

    // Final merge
    Merge(left, leftSize, right, rightSize, Compare);
}

}
}

