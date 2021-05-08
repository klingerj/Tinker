#pragma once

#include "Core/CoreDefines.h"
#include "Core/Mem.h"

namespace Tinker
{
namespace Core
{

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

        // Assuming that left and right are actually contiguous
        T* dstList = left + i + j;
        if (Compare(tmpList[i], tmpList[numElesLeft + j]))
        {
            memcpy(dstList, &tmpList[i], sizeof(T));
            ++i;
        }
        else
        {
            memcpy(dstList, &tmpList[numElesLeft + j], sizeof(T));
            ++j;
        }

        // Check if one counter finishes before the other
        if (i == numElesLeft)
        {
            uint32 numRightRemaining = numElesRight - j;
            memcpy(left + i + j, &tmpList[j], sizeof(T) * numRightRemaining);
            break;
        }
        else if (j == numElesRight)
        {
            uint32 numLeftRemaining = numElesLeft - i;
            memcpy(left + i + j, &tmpList[i], sizeof(T) * numLeftRemaining);
            break;
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

