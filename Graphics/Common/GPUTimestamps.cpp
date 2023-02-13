#include "GPUTimestamps.h"
#include "GraphicsCommon.h"

namespace Tk
{
namespace Graphics
{
namespace GPUTimestamps
{

static uint64 gpuTimestampCPUCopy         [GPU_TIMESTAMP_NUM_MAX];
static float  gpuTimestampCPUCopyProcessed[GPU_TIMESTAMP_NUM_MAX - 1];

static float totalFrameTimeThisFrame = 0.0f;

static uint32 numGPUTimestampsRecorded[MAX_FRAMES_IN_FLIGHT] = {};

void IncrementTimestampCount()
{
    ++numGPUTimestampsRecorded[GetCurrentFrameInFlightIndex()];
    TINKER_ASSERT(numGPUTimestampsRecorded[GetCurrentFrameInFlightIndex()] <= GPU_TIMESTAMP_NUM_MAX);
}

void* GetRawCPUSideTimestampBuffer()
{
    return gpuTimestampCPUCopy;
}

uint32 GetMostRecentRecordedTimestampCount()
{
    return numGPUTimestampsRecorded[GetCurrentFrameInFlightIndex()];
}

void ProcessTimestamps()
{
    if (numGPUTimestampsRecorded[GetCurrentFrameInFlightIndex()] < 1)
    {
        return;
    }

    // Convert ticks to time
    const double microsecondsPerTick = 1e-3 * GetGPUTimestampPeriod();

    // Calc total time
    totalFrameTimeThisFrame = (float)(microsecondsPerTick * (double)(gpuTimestampCPUCopy[numGPUTimestampsRecorded[GetCurrentFrameInFlightIndex()] - 1] - gpuTimestampCPUCopy[0]));

    for (uint32 i = 1; i < numGPUTimestampsRecorded[GetCurrentFrameInFlightIndex()]; ++i)
    {
        gpuTimestampCPUCopyProcessed[i - 1] = (float)(microsecondsPerTick * (double)(gpuTimestampCPUCopy[i] - gpuTimestampCPUCopy[i - 1]));
    }

    // reset
    numGPUTimestampsRecorded[GetCurrentFrameInFlightIndex()] = 0;
}

float GetTimestampValueByID(uint32 timestampID)
{
    TINKER_ASSERT(timestampID < ARRAYCOUNT(gpuTimestampCPUCopy));
    return gpuTimestampCPUCopyProcessed[timestampID];
}

}
}
}
