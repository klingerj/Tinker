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

static uint32 gpuTimestampsRecordedThisFrame = 0;

void Reset()
{
    gpuTimestampsRecordedThisFrame = 0;
}

void IncrementTimestampCount()
{
    ++gpuTimestampsRecordedThisFrame;
    TINKER_ASSERT(gpuTimestampsRecordedThisFrame <= GPU_TIMESTAMP_NUM_MAX);
}

void* GetRawCPUSideTimestampBuffer()
{
    return gpuTimestampCPUCopy;
}

void ProcessTimestamps()
{
    if (gpuTimestampsRecordedThisFrame < 1)
    {
        return;
    }

    // Convert ticks to time
    const double microsecondsPerTick = 1e-3 * GetGPUTimestampPeriod();

    // Calc total time
    totalFrameTimeThisFrame = (float)(microsecondsPerTick * (double)(gpuTimestampCPUCopy[gpuTimestampsRecordedThisFrame - 1] - gpuTimestampCPUCopy[0]));

    for (uint32 i = 1; i < gpuTimestampsRecordedThisFrame; ++i)
    {
        gpuTimestampCPUCopyProcessed[i - 1] = (float)(microsecondsPerTick * (double)(gpuTimestampCPUCopy[i] - gpuTimestampCPUCopy[i - 1]));
    }
}

float GetTimestampValueByID(uint32 timestampID)
{
    TINKER_ASSERT(timestampID < ARRAYCOUNT(gpuTimestampCPUCopy));
    return gpuTimestampCPUCopyProcessed[timestampID];
}

}
}
}
