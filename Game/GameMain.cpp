#include "../Include/PlatformGameAPI.h"
#include "../Include/Core/Allocators.h"
#include "../Include/Core/Math/VectorTypes.h"
#include "../Include/Core/Containers/RingBuffer.h"
#include "GameGraphicsTypes.h"

#include <cstring>
#include <string.h>
#include <vector>

/*static void Test_Thread_Func()
{
    Tinker::Platform::PrintDebugString("I am from a thread.\n");
}
*/

DefaultGeometry<4, 6> defaultQuad = {
    TINKER_INVALID_HANDLE,
    TINKER_INVALID_HANDLE,
    TINKER_INVALID_HANDLE,
    TINKER_INVALID_HANDLE,
    v4f(-1.0f, -1.0f, 0.0f, 1.0f),
    v4f(1.0f, -1.0f, 0.0f, 1.0f),
    v4f(-1.0f, 1.0f, 0.0f, 1.0f),
    v4f(1.0f, 1.0f, 0.0f, 1.0f),
    0, 1, 2, 2, 1, 3
};

static GameGraphicsData gameGraphicsData = {};
uint32 gameWidth = 1920;
uint32 gameHeight = 1080;

static std::vector<Tinker::Platform::GraphicsCommand> graphicsCommands;

static bool isGameInitted = false;
static const bool isMultiplayer = false;
static bool connectedToServer = false;

extern "C"
GAME_UPDATE(GameUpdate)
{
    graphicsCommands.clear();

    if (!isGameInitted)
    {
        // Init network connection if multiplayer
        if (isMultiplayer)
        {
            int result = platformFuncs->InitNetworkConnection();
            if (result != 0)
            {
                connectedToServer = false;
                return 1;
            }
            else
            {
                connectedToServer = true;
            }
        }

        graphicsCommands.reserve(graphicsCommandStream->m_maxCommands);

        // Default geometry
        defaultQuad.m_vertexBufferHandle = platformFuncs->CreateVertexBuffer(sizeof(defaultQuad.m_points), Tinker::Platform::Graphics::BufferType::eVertexBuffer);
        defaultQuad.m_indexBufferHandle = platformFuncs->CreateVertexBuffer(sizeof(defaultQuad.m_indices), Tinker::Platform::Graphics::BufferType::eIndexBuffer);
        Tinker::Platform::Graphics::StagingBufferData stagingBufferData = platformFuncs->CreateStagingBuffer(sizeof(defaultQuad.m_points));
        defaultQuad.m_stagingBufferHandle_vert = stagingBufferData.handle;
        memcpy(stagingBufferData.memory, defaultQuad.m_points, sizeof(defaultQuad.m_points));
        stagingBufferData = platformFuncs->CreateStagingBuffer(sizeof(defaultQuad.m_indices));
        defaultQuad.m_stagingBufferHandle_idx = stagingBufferData.handle;
        memcpy(stagingBufferData.memory, defaultQuad.m_indices, sizeof(defaultQuad.m_points));

        Tinker::Platform::GraphicsCommand command;
        command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdMemTransfer;
        command.m_sizeInBytes = sizeof(defaultQuad.m_points);
        command.m_dstBufferHandle = defaultQuad.m_vertexBufferHandle;
        command.m_srcBufferHandle = defaultQuad.m_stagingBufferHandle_vert;
        graphicsCommands.push_back(command);
        command.m_sizeInBytes = sizeof(defaultQuad.m_indices);
        command.m_dstBufferHandle = defaultQuad.m_indexBufferHandle;
        command.m_srcBufferHandle = defaultQuad.m_stagingBufferHandle_idx;
        graphicsCommands.push_back(command);

        // Game graphics
        uint32 numVertBytes = sizeof(v4f) * 4; // aligned memory size
        uint32 numIdxBytes = sizeof(uint32) * 16; // aligned memory size
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

        gameGraphicsData.m_imageHandle = platformFuncs->CreateImageResource(gameWidth, gameHeight);
        gameGraphicsData.m_imageViewHandle = platformFuncs->CreateImageViewResource(gameGraphicsData.m_imageHandle);
        gameGraphicsData.m_framebufferHandle = platformFuncs->CreateFramebuffer(&gameGraphicsData.m_imageViewHandle, 1, gameWidth, gameHeight);

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
    Tinker::Platform::WorkerJob* jobs[32] = {};
    for (uint32 i = 0; i < 32; ++i)
    {
        jobs[i] = Tinker::Platform::CreateNewThreadJob([=]()
                {
                    // TODO: custom string library...
                    char* string = "I am from a thread";
                    char dst[50] = "";
                    strcpy_s(dst, string);
                    _itoa_s(i, dst + strlen(string), 10, 10);
                    if (i < 10) dst[strlen(string) + 1] = ' ';
                    dst[strlen(string) + 2] = '\n';
                    dst[strlen(string) + 3] = '\0';
                    Tinker::Platform::PrintDebugString(dst);
                });
        platformFuncs->EnqueueWorkerThreadJob(jobs[i]);
    }
    for (uint32 i = 0; i < 32; ++i)
    {
        platformFuncs->WaitOnThreadJob(jobs[i]);
    }
    
    // Issue graphics commands
    Tinker::Platform::GraphicsCommand command;

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdMemTransfer;
    command.m_sizeInBytes = numVertBytes;
    command.m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle;
    command.m_dstBufferHandle = gameGraphicsData.m_vertexBufferHandle;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdMemTransfer;
    command.m_sizeInBytes = numVertBytes;
    command.m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle2;
    command.m_dstBufferHandle = gameGraphicsData.m_vertexBufferHandle2;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdMemTransfer;
    command.m_sizeInBytes = numIdxBytes;
    command.m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle3;
    command.m_dstBufferHandle = gameGraphicsData.m_indexBufferHandle;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdMemTransfer;
    command.m_sizeInBytes = numIdxBytes;
    command.m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle4;
    command.m_dstBufferHandle = gameGraphicsData.m_indexBufferHandle2;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassBegin;
    command.m_renderPassHandle = TINKER_INVALID_HANDLE;
    command.m_framebufferHandle = gameGraphicsData.m_framebufferHandle;
    command.m_renderWidth = gameWidth;
    command.m_renderHeight = gameHeight;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    command.m_numIndices = 3;
    command.m_numUVs = 0;
    command.m_numVertices = 3;
    command.m_indexBufferHandle = gameGraphicsData.m_indexBufferHandle;
    command.m_vertexBufferHandle = gameGraphicsData.m_vertexBufferHandle;
    command.m_uvBufferHandle = TINKER_INVALID_HANDLE;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    command.m_numIndices = 3;
    command.m_numUVs = 0;
    command.m_numVertices = 3;
    command.m_indexBufferHandle = gameGraphicsData.m_indexBufferHandle2;
    command.m_vertexBufferHandle = gameGraphicsData.m_vertexBufferHandle2;
    command.m_uvBufferHandle = TINKER_INVALID_HANDLE;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassEnd;
    command.m_renderPassHandle = TINKER_INVALID_HANDLE;
    graphicsCommands.push_back(command);
    
    // Blit to screen
    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassBegin;
    command.m_renderPassHandle = TINKER_INVALID_HANDLE;
    command.m_framebufferHandle = TINKER_INVALID_HANDLE;
    command.m_renderWidth = 0;
    command.m_renderHeight = 0;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    command.m_numIndices = 3;
    command.m_numUVs = 0;
    command.m_numVertices = 3;
    /*command.m_indexBufferHandle = defaultQuad.m_indexBufferHandle;
    command.m_vertexBufferHandle = defaultQuad.m_vertexBufferHandle;*/
    command.m_indexBufferHandle = gameGraphicsData.m_indexBufferHandle2;
    command.m_vertexBufferHandle = gameGraphicsData.m_vertexBufferHandle2;
    command.m_uvBufferHandle = TINKER_INVALID_HANDLE;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassEnd;
    command.m_renderPassHandle = TINKER_INVALID_HANDLE;
    graphicsCommands.push_back(command);

    graphicsCommandStream->m_numCommands = (uint32)graphicsCommands.size();
    Tinker::Platform::GraphicsCommand* graphicsCmdBase = graphicsCommandStream->m_graphicsCommands;
    uint32 graphicsCmdSizeInBytes = (uint32)graphicsCommands.size() * sizeof(Tinker::Platform::GraphicsCommand);
    memcpy(graphicsCmdBase, graphicsCommands.data(), graphicsCmdSizeInBytes);
    graphicsCmdBase += graphicsCmdSizeInBytes;

    if (isGameInitted && isMultiplayer && connectedToServer)
    {
        int result = platformFuncs->SendMessageToServer();
        if (result != 0)
        {
            return 1;
        }
        else
        {
            // Send message successfully
        }
    }

    return 0;
}

GAME_DESTROY(GameDestroy)
{
    if (isGameInitted)
    {
        // Default geometry
        platformFuncs->DestroyVertexBuffer(defaultQuad.m_vertexBufferHandle);
        platformFuncs->DestroyVertexBuffer(defaultQuad.m_indexBufferHandle);
        platformFuncs->DestroyStagingBuffer(defaultQuad.m_stagingBufferHandle_vert);
        platformFuncs->DestroyStagingBuffer(defaultQuad.m_stagingBufferHandle_idx);

        // Game graphics
        platformFuncs->DestroyVertexBuffer(gameGraphicsData.m_vertexBufferHandle);
        platformFuncs->DestroyVertexBuffer(gameGraphicsData.m_vertexBufferHandle2);
        platformFuncs->DestroyStagingBuffer(gameGraphicsData.m_stagingBufferHandle);
        platformFuncs->DestroyStagingBuffer(gameGraphicsData.m_stagingBufferHandle2);
        platformFuncs->DestroyVertexBuffer(gameGraphicsData.m_indexBufferHandle);
        platformFuncs->DestroyVertexBuffer(gameGraphicsData.m_indexBufferHandle2);
        platformFuncs->DestroyStagingBuffer(gameGraphicsData.m_stagingBufferHandle3);
        platformFuncs->DestroyStagingBuffer(gameGraphicsData.m_stagingBufferHandle4);

        platformFuncs->DestroyFramebuffer(gameGraphicsData.m_framebufferHandle);
        platformFuncs->DestroyImageResource(gameGraphicsData.m_imageHandle);
        platformFuncs->DestroyImageViewResource(gameGraphicsData.m_imageViewHandle);
    }
}
