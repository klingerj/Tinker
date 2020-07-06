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

    uint32 m_vertexBufferHandle2;
    uint32 m_stagingBufferHandle2;
    void* m_stagingBufferMemPtr2;
} GameGraphicsData;

static GameGraphicsData gameGraphicsData = {};

static bool isGameInitted = false;

extern "C"
GAME_UPDATE(GameUpdate)
{
    if (!isGameInitted)
    {
        uint32 numVertBytes = sizeof(v4f) * 3;
        uint32 vertexBufferHandle = platformFuncs->CreateVertexBuffer(numVertBytes);
        uint32 stagingBufferHandle = platformFuncs->CreateStagingBuffer(numVertBytes);
        void* stagingBufferMemPtr = platformFuncs->GetStagingBufferMemory(stagingBufferHandle);

        uint32 vertexBufferHandle2 = platformFuncs->CreateVertexBuffer(numVertBytes);
        uint32 stagingBufferHandle2 = platformFuncs->CreateStagingBuffer(numVertBytes);
        void* stagingBufferMemPtr2 = platformFuncs->GetStagingBufferMemory(stagingBufferHandle2);

        gameGraphicsData.m_vertexBufferHandle = vertexBufferHandle;
        gameGraphicsData.m_stagingBufferHandle = stagingBufferHandle;
        gameGraphicsData.m_stagingBufferMemPtr = stagingBufferMemPtr;

        gameGraphicsData.m_vertexBufferHandle2 = vertexBufferHandle2;
        gameGraphicsData.m_stagingBufferHandle2 = stagingBufferHandle2;
        gameGraphicsData.m_stagingBufferMemPtr2 = stagingBufferMemPtr2;

        isGameInitted = true;
    }

    uint32 numVertBytes = sizeof(v4f) * 3;
    v4f positions[] = { v4f(0.0f, -0.5f, 0.0f, 1.0f), v4f(0.5f, 0.5f, 0.0f, 1.0f), v4f(-0.5f, 0.5f, 0.0f, 1.0f) };
    memcpy(gameGraphicsData.m_stagingBufferMemPtr, positions, numVertBytes);

    v4f positions2[] = { v4f(0.45f, -0.5f, 0.0f, 1.0f), v4f(0.75f, 0.25f, 0.0f, 1.0f), v4f(0.15f, 0.25f, 0.0f, 1.0f) };
    memcpy(gameGraphicsData.m_stagingBufferMemPtr2, positions2, numVertBytes);

    Tinker::Platform::PrintDebugString("Joe\n");

    // Test a thread job
    Tinker::Platform::WorkerJob* job = Tinker::Platform::CreateNewThreadJob(Test_Thread_Func);
    platformFuncs->EnqueueWorkerThreadJob(job);
    Tinker::Platform::WaitOnJob(job);

    // Issue a test draw command
    const uint32 numCommands = 6;
    Tinker::Platform::GraphicsCommand testCmd[numCommands];
    uint32 commandsSize = sizeof(Tinker::Platform::GraphicsCommand) * numCommands;

    testCmd[0].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdMemTransfer;
    testCmd[0].m_dstBufferType = 0;
    testCmd[0].m_sizeInBytes = numVertBytes;
    testCmd[0].m_srcStagingBufferHandle = gameGraphicsData.m_stagingBufferHandle;
    testCmd[0].m_dstIndexBufferHandle = 0xffffffff;
    testCmd[0].m_dstVertexBufferHandle = gameGraphicsData.m_vertexBufferHandle;
    testCmd[0].m_uvBufferHandle = 0xffffffff;

    testCmd[1].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdMemTransfer;
    testCmd[1].m_dstBufferType = 0;
    testCmd[1].m_sizeInBytes = numVertBytes;
    testCmd[1].m_srcStagingBufferHandle = gameGraphicsData.m_stagingBufferHandle2;
    testCmd[1].m_dstIndexBufferHandle = 0xffffffff;
    testCmd[1].m_dstVertexBufferHandle = gameGraphicsData.m_vertexBufferHandle2;
    testCmd[1].m_uvBufferHandle = 0xffffffff;

    testCmd[2].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassBegin;
    testCmd[2].m_renderPassHandle = 0xffffffff;

    testCmd[3].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    testCmd[3].m_numIndices = 0;
    testCmd[3].m_numUVs = 0;
    testCmd[3].m_numVertices = 3;
    testCmd[3].m_indexBufferHandle = 0xffffffff;
    testCmd[3].m_vertexBufferHandle = gameGraphicsData.m_vertexBufferHandle;
    testCmd[3].m_uvBufferHandle = 0xffffffff;

    testCmd[4].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    testCmd[4].m_numIndices = 0;
    testCmd[4].m_numUVs = 0;
    testCmd[4].m_numVertices = 3;
    testCmd[4].m_indexBufferHandle = 0xffffffff;
    testCmd[4].m_vertexBufferHandle = gameGraphicsData.m_vertexBufferHandle2;
    testCmd[4].m_uvBufferHandle = 0xffffffff;

    testCmd[5].m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassEnd;
    testCmd[5].m_renderPassHandle = 0xffffffff;
    
    graphicsCommandStream->m_numCommands = numCommands;
    Tinker::Platform::GraphicsCommand* graphicsCmdBase = graphicsCommandStream->m_graphicsCommands;
    memcpy(graphicsCmdBase, testCmd, commandsSize);
    graphicsCmdBase += commandsSize;
    
    /*Tinker::Memory::PoolAllocator<v4f> poolAllocator;
    poolAllocator.Init(2048, 1);
    uint32 firstEle = poolAllocator.Alloc();
    uint32 secondEle = poolAllocator.Alloc();
    uint32 thirdEle = poolAllocator.Alloc();

    v4f* vec = poolAllocator.PtrFromHandle(firstEle);
    vec->x = 1.0f;
    vec->y = 2.0f;
    vec->z = 3.0f;
    vec->w = 4.0f;

    poolAllocator.Dealloc(secondEle);
    poolAllocator.Dealloc(firstEle);*/

    return 0;
}
