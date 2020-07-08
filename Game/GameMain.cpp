#include "../Include/PlatformGameAPI.h"
#include "../Include/Core/Allocators.h"
#include "../Include/Core/Math/VectorTypes.h"
#include "../Include/Core/Containers/RingBuffer.h"

#include <cstring>

static void Test_Thread_Func()
{
    Tinker::Platform::PrintDebugString("I am from a thread.\n");
}

typedef struct game_graphic_data
{
    uint32 m_vertexBufferHandle;
    uint32 m_stagingBufferHandle;
    void* m_stagingBufferMemPtr;
    uint32 m_indexBufferHandle;
    uint32 m_stagingBufferHandle3;
    void* m_stagingBufferMemPtr3;

    uint32 m_vertexBufferHandle2;
    uint32 m_stagingBufferHandle2;
    void* m_stagingBufferMemPtr2;
    uint32 m_indexBufferHandle2;
    uint32 m_stagingBufferHandle4;
    void* m_stagingBufferMemPtr4;
} GameGraphicsData;

static GameGraphicsData gameGraphicsData = {};

static bool isGameInitted = false;

extern "C"
GAME_UPDATE(GameUpdate)
{
    if (!isGameInitted)
    {
        uint32 numVertBytes = sizeof(v4f) * 4; // aligned memory size
        uint32 numIdxBytes = sizeof(uint32) * 4; // aligned memory size
        uint32 vertexBufferHandle = platformFuncs->CreateVertexBuffer(numVertBytes, Tinker::Platform::Graphics::BufferType::eVertexBuffer);
        Tinker::Platform::Graphics::StagingBufferData data = platformFuncs->CreateStagingBuffer(numVertBytes);
        uint32 stagingBufferHandle = data.handle;
        void* stagingBufferMemPtr = data.memory;
        uint32 indexBufferHandle = platformFuncs->CreateVertexBuffer(numIdxBytes, Tinker::Platform::Graphics::BufferType::eIndexBuffer);
        data = platformFuncs->CreateStagingBuffer(numIdxBytes);
        uint32 stagingBufferHandle3 = data.handle;
        void* stagingBufferMemPtr3 = data.memory;

        uint32 vertexBufferHandle2 = platformFuncs->CreateVertexBuffer(numVertBytes, Tinker::Platform::Graphics::BufferType::eVertexBuffer);
        data = platformFuncs->CreateStagingBuffer(numVertBytes);
        uint32 stagingBufferHandle2 = data.handle;
        void* stagingBufferMemPtr2 = data.memory;
        uint32 indexBufferHandle2 = platformFuncs->CreateVertexBuffer(numIdxBytes, Tinker::Platform::Graphics::BufferType::eIndexBuffer);
        data = platformFuncs->CreateStagingBuffer(numIdxBytes);
        uint32 stagingBufferHandle4 = data.handle;
        void* stagingBufferMemPtr4 = data.memory;

        gameGraphicsData.m_vertexBufferHandle = vertexBufferHandle;
        gameGraphicsData.m_stagingBufferHandle = stagingBufferHandle;
        gameGraphicsData.m_stagingBufferMemPtr = stagingBufferMemPtr;
        gameGraphicsData.m_indexBufferHandle = indexBufferHandle;
        gameGraphicsData.m_stagingBufferHandle3 = stagingBufferHandle3;
        gameGraphicsData.m_stagingBufferMemPtr3 = stagingBufferMemPtr3;

        gameGraphicsData.m_vertexBufferHandle2 = vertexBufferHandle2;
        gameGraphicsData.m_stagingBufferHandle2 = stagingBufferHandle2;
        gameGraphicsData.m_stagingBufferMemPtr2 = stagingBufferMemPtr2;
        gameGraphicsData.m_indexBufferHandle2 = indexBufferHandle2;
        gameGraphicsData.m_stagingBufferHandle4 = stagingBufferHandle4;
        gameGraphicsData.m_stagingBufferMemPtr4 = stagingBufferMemPtr4;

        isGameInitted = true;
    }

    uint32 numVertBytes = sizeof(v4f) * 3;
    uint32 numIdxBytes = sizeof(uint32) * 3;
    v4f positions[] = { v4f(0.0f, -0.5f, 0.0f, 1.0f), v4f(0.5f, 0.5f, 0.0f, 1.0f), v4f(-0.5f, 0.5f, 0.0f, 1.0f) };
    uint32 indices[] = { 0, 2, 1 };
    memcpy(gameGraphicsData.m_stagingBufferMemPtr, positions, numVertBytes);
    memcpy(gameGraphicsData.m_stagingBufferMemPtr3, indices, numIdxBytes);

    v4f positions2[] = { v4f(0.45f, -0.5f, 0.0f, 1.0f), v4f(0.75f, 0.25f, 0.0f, 1.0f), v4f(0.15f, 0.25f, 0.0f, 1.0f) };
    uint32 indices2[] = { 0, 2, 1 };
    memcpy(gameGraphicsData.m_stagingBufferMemPtr2, positions2, numVertBytes);
    memcpy(gameGraphicsData.m_stagingBufferMemPtr4, indices2, numIdxBytes);

    Tinker::Platform::PrintDebugString("Joe\n");

    // Test a thread job
    Tinker::Platform::WorkerJob* job = Tinker::Platform::CreateNewThreadJob(Test_Thread_Func);
    platformFuncs->EnqueueWorkerThreadJob(job);
    Tinker::Platform::WaitOnJob(job);

    // Issue a test draw command
    const uint32 numCommands = 8;
    Tinker::Platform::GraphicsCommand testCmd[numCommands];
    uint32 commandsSize = sizeof(Tinker::Platform::GraphicsCommand) * numCommands;

    testCmd[0].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdMemTransfer;
    testCmd[0].m_sizeInBytes = numVertBytes;
    testCmd[0].m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle;
    testCmd[0].m_dstBufferHandle = gameGraphicsData.m_vertexBufferHandle;

    testCmd[1].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdMemTransfer;
    testCmd[1].m_sizeInBytes = numVertBytes;
    testCmd[1].m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle2;
    testCmd[1].m_dstBufferHandle = gameGraphicsData.m_vertexBufferHandle2;

    testCmd[2].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdMemTransfer;
    testCmd[2].m_sizeInBytes = numIdxBytes;
    testCmd[2].m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle3;
    testCmd[2].m_dstBufferHandle = gameGraphicsData.m_indexBufferHandle;

    testCmd[3].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdMemTransfer;
    testCmd[3].m_sizeInBytes = numIdxBytes;
    testCmd[3].m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle4;
    testCmd[3].m_dstBufferHandle = gameGraphicsData.m_indexBufferHandle2;

    testCmd[4].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassBegin;
    testCmd[4].m_renderPassHandle = 0xffffffff;

    testCmd[5].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    testCmd[5].m_numIndices = 3;
    testCmd[5].m_numUVs = 0;
    testCmd[5].m_numVertices = 3;
    testCmd[5].m_indexBufferHandle = gameGraphicsData.m_indexBufferHandle;
    testCmd[5].m_vertexBufferHandle = gameGraphicsData.m_vertexBufferHandle;
    testCmd[5].m_uvBufferHandle = 0xffffffff;

    testCmd[6].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    testCmd[6].m_numIndices = 3;
    testCmd[6].m_numUVs = 0;
    testCmd[6].m_numVertices = 3;
    testCmd[6].m_indexBufferHandle = gameGraphicsData.m_indexBufferHandle2;
    testCmd[6].m_vertexBufferHandle = gameGraphicsData.m_vertexBufferHandle2;
    testCmd[6].m_uvBufferHandle = 0xffffffff;

    testCmd[7].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassEnd;
    testCmd[7].m_renderPassHandle = 0xffffffff;
    
    graphicsCommandStream->m_numCommands = numCommands;
    Tinker::Platform::GraphicsCommand* graphicsCmdBase = graphicsCommandStream->m_graphicsCommands;
    memcpy(graphicsCmdBase, testCmd, commandsSize);
    graphicsCmdBase += commandsSize;

    return 0;
}

GAME_DESTROY(GameDestroy)
{
    if (isGameInitted)
    {
        platformFuncs->DestroyVertexBuffer(gameGraphicsData.m_vertexBufferHandle);
        platformFuncs->DestroyVertexBuffer(gameGraphicsData.m_vertexBufferHandle2);
        platformFuncs->DestroyStagingBuffer(gameGraphicsData.m_stagingBufferHandle);
        platformFuncs->DestroyStagingBuffer(gameGraphicsData.m_stagingBufferHandle2);
        platformFuncs->DestroyVertexBuffer(gameGraphicsData.m_indexBufferHandle);
        platformFuncs->DestroyVertexBuffer(gameGraphicsData.m_indexBufferHandle2);
        platformFuncs->DestroyStagingBuffer(gameGraphicsData.m_stagingBufferHandle3);
        platformFuncs->DestroyStagingBuffer(gameGraphicsData.m_stagingBufferHandle4);
    }
}
