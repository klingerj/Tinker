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

#define DEFAULT_QUAD_NUM_VERTICES 4
#define DEFAULT_QUAD_NUM_INDICES 6
DefaultGeometry<DEFAULT_QUAD_NUM_VERTICES, DEFAULT_QUAD_NUM_INDICES> defaultQuad = {
    TINKER_INVALID_HANDLE,
    TINKER_INVALID_HANDLE,
    TINKER_INVALID_HANDLE,
    TINKER_INVALID_HANDLE,
    v4f(-1.0f, -1.0f, 0.0f, 1.0f),
    v4f(1.0f, -1.0f, 0.0f, 1.0f),
    v4f(-1.0f, 1.0f, 0.0f, 1.0f),
    v4f(1.0f, 1.0f, 0.0f, 1.0f),
    //0, 1, 2, 2, 1, 3
    2, 1, 0, 3, 1, 2
};

static GameGraphicsData gameGraphicsData = {};

static std::vector<Tinker::Platform::GraphicsCommand> graphicsCommands;

static bool isGameInitted = false;
static const bool isMultiplayer = false;
static bool connectedToServer = false;

uint32 currentWindowWidth, currentWindowHeight;

Tinker::Memory::LinearAllocator shaderBytecodeAllocator;
const uint32 totalShaderBytecodeMaxSizeInBytes = 1024 * 10;

typedef struct current_input_state
{
    Tinker::Platform::KeycodeState keyCodes[Tinker::Platform::eMaxKeycodes];
} CurrentInputState;
static CurrentInputState currentInputState = {};
static CurrentInputState previousInputState = {};

uint32 LoadShader(const Tinker::Platform::PlatformAPIFuncs* platformFuncs, const char* vertexShaderFileName, const char* fragmentShaderFileName, Tinker::Platform::GraphicsPipelineParams* params)
{
    uint32 vertexShaderFileSize = platformFuncs->GetFileSize(vertexShaderFileName);
    uint32 fragmentShaderFileSize = platformFuncs->GetFileSize(fragmentShaderFileName);
    TINKER_ASSERT(vertexShaderFileSize > 0);
    TINKER_ASSERT(fragmentShaderFileSize > 0);

    uint8* vertexShaderBuffer = shaderBytecodeAllocator.Alloc(vertexShaderFileSize, 1);
    uint8* fragmentShaderBuffer = shaderBytecodeAllocator.Alloc(fragmentShaderFileSize, 1);
    void* vertexShaderCode = platformFuncs->ReadEntireFile(vertexShaderFileName, vertexShaderFileSize, vertexShaderBuffer);
    void* fragmentShaderCode = platformFuncs->ReadEntireFile(fragmentShaderFileName, fragmentShaderFileSize, fragmentShaderBuffer);

    return platformFuncs->CreateGraphicsPipeline(vertexShaderCode, vertexShaderFileSize, fragmentShaderCode, fragmentShaderFileSize, params->blendState, params->depthState, params->viewportWidth, params->viewportHeight, params->renderPassHandle, params->descriptorHandle);
}

void LoadAllShaders(const Tinker::Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    shaderBytecodeAllocator.Free();
    shaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);

    Tinker::Platform::DescriptorLayout mainDrawDescriptorLayout = {};
    Tinker::Platform::InitDescLayout(&mainDrawDescriptorLayout);
    Tinker::Platform::GraphicsPipelineParams params;
    params.blendState = Tinker::Platform::eBlendStateAlphaBlend;
    params.depthState = Tinker::Platform::eDepthStateTestOnWriteOn;
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.renderPassHandle = gameGraphicsData.m_mainRenderPassHandle;
    params.descriptorHandle = TINKER_INVALID_HANDLE;
    gameGraphicsData.m_shaderHandle = LoadShader(platformFuncs, "..\\Shaders\\spv\\basic_glsl_vert.spv", "..\\Shaders\\spv\\basic_glsl_frag.spv", &params);

    params.blendState = Tinker::Platform::eBlendStateAlphaBlend;
    params.depthState = Tinker::Platform::eDepthStateOff;
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.renderPassHandle = TINKER_INVALID_HANDLE;
    params.descriptorHandle = gameGraphicsData.m_swapChainBlitDescHandle;
    gameGraphicsData.m_blitShaderHandle = LoadShader(platformFuncs, "..\\Shaders\\spv\\blit_glsl_vert.spv", "..\\Shaders\\spv\\blit_glsl_frag.spv", &params);
}

void DestroyShaders(const Tinker::Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_shaderHandle);
    platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_blitShaderHandle);
    gameGraphicsData.m_shaderHandle = TINKER_INVALID_HANDLE;
    gameGraphicsData.m_blitShaderHandle = TINKER_INVALID_HANDLE;
}

void DestroyDescriptors(const Tinker::Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyDescriptor(gameGraphicsData.m_swapChainBlitDescHandle);
    gameGraphicsData.m_swapChainBlitDescHandle = TINKER_INVALID_HANDLE;

    platformFuncs->DestroyAllDescriptors(); // TODO: this is not a good API and should be per-pool or something
}

void CreateAllDescriptors(const Tinker::Platform::PlatformAPIFuncs* platformFuncs)
{
    Tinker::Platform::DescriptorLayout blitDescriptorLayout = {};
    Tinker::Platform::InitDescLayout(&blitDescriptorLayout);
    blitDescriptorLayout.descriptorTypes[0][0].type = Tinker::Platform::eDescriptorTypeSampledImage;
    blitDescriptorLayout.descriptorTypes[0][0].amount = 1;
    gameGraphicsData.m_swapChainBlitDescHandle = platformFuncs->CreateDescriptor(&blitDescriptorLayout);

    Tinker::Platform::DescriptorSetDataHandles blitHandles = {};
    blitHandles.handles[0] = gameGraphicsData.m_imageViewHandle;
    platformFuncs->WriteDescriptor(&blitDescriptorLayout, gameGraphicsData.m_swapChainBlitDescHandle, &blitHandles);
}

void RecreateShaders(const Tinker::Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    DestroyShaders(platformFuncs);
    
    DestroyDescriptors(platformFuncs);

    CreateAllDescriptors(platformFuncs);
    LoadAllShaders(platformFuncs, windowWidth, windowHeight);
}

void CreateGameRenderingResources(const Tinker::Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    gameGraphicsData.m_imageHandle = platformFuncs->CreateImageResource(windowWidth, windowHeight);
    gameGraphicsData.m_imageViewHandle = platformFuncs->CreateImageViewResource(gameGraphicsData.m_imageHandle);
    gameGraphicsData.m_mainRenderPassHandle = platformFuncs->CreateRenderPass(Tinker::Platform::eImageLayoutUndefined, Tinker::Platform::eImageLayoutShaderRead);
    gameGraphicsData.m_framebufferHandle = platformFuncs->CreateFramebuffer(&gameGraphicsData.m_imageViewHandle, 1, windowWidth, windowHeight, gameGraphicsData.m_mainRenderPassHandle);
}

void ProcessInputState(const Tinker::Platform::InputStateDeltas* inputStateDeltas, const Tinker::Platform::PlatformAPIFuncs* platformFuncs)
{
    previousInputState = currentInputState;

    for (uint32 uiKeycode = 0; uiKeycode < Tinker::Platform::eMaxKeycodes; ++uiKeycode)
    {
        if (inputStateDeltas->keyCodes[uiKeycode].numStateChanges > 0)
        {
            currentInputState.keyCodes[uiKeycode].isDown = inputStateDeltas->keyCodes[uiKeycode].isDown;
            currentInputState.keyCodes[uiKeycode].numStateChanges += inputStateDeltas->keyCodes[uiKeycode].numStateChanges;
            // TODO: reset number of state changes at some point, every second or every several frames or something
        }
    }

    for (uint32 uiKeycode = 0; uiKeycode < Tinker::Platform::eMaxKeycodes; ++uiKeycode)
    {
        switch (uiKeycode)
        {
            case Tinker::Platform::eKeyF10:
            {
                // Handle the initial downpress once and nothing more
                if (currentInputState.keyCodes[uiKeycode].isDown && !previousInputState.keyCodes[uiKeycode].isDown)
                {
                    Tinker::Platform::PrintDebugString("Hotloading Shaders...\n");
                    RecreateShaders(platformFuncs, currentWindowWidth, currentWindowHeight);
                    Tinker::Platform::PrintDebugString("...Done.\n");
                }

                // Handle as long as the key is down
                if (currentInputState.keyCodes[uiKeycode].isDown)
                {
                }
                
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

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

        CreateGameRenderingResources(platformFuncs, windowWidth, windowHeight);

        CreateAllDescriptors(platformFuncs);
        LoadAllShaders(platformFuncs, windowWidth, windowHeight);

        isGameInitted = true;
    }

    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;

    ProcessInputState(inputStateDeltas, platformFuncs);

    
    /*static uint32 shaderHotloadCounter = 0;
    ++shaderHotloadCounter;
    if (shaderHotloadCounter % 2000 == 100)
    {
        Tinker::Platform::PrintDebugString("Hotloading Shaders...\n");
        RecreateShaders(platformFuncs, windowWidth, windowHeight);
        Tinker::Platform::PrintDebugString("...Done.\n");
    }*/

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

    //Tinker::Platform::PrintDebugString("Joe\n");

    // Test a thread job
    /*Tinker::Platform::WorkerJob* jobs[32] = {};
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
    }*/
    
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

    /*command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdImageCopy;
    command.m_srcImgHandle = gameGraphicsData.m_imageHandle;
    command.m_dstImgHandle = TINKER_INVALID_HANDLE;
    command.m_width = windowWidth;
    command.m_height = windowHeight;
    graphicsCommands.push_back(command);*/

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassBegin;
    command.m_renderPassHandle = gameGraphicsData.m_mainRenderPassHandle;
    command.m_framebufferHandle = gameGraphicsData.m_framebufferHandle;
    command.m_renderWidth = windowWidth;
    command.m_renderHeight = windowHeight;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    command.m_numIndices = 3;
    command.m_numUVs = 0;
    command.m_numVertices = 3;
    command.m_indexBufferHandle = gameGraphicsData.m_indexBufferHandle;
    command.m_vertexBufferHandle = gameGraphicsData.m_vertexBufferHandle;
    command.m_uvBufferHandle = TINKER_INVALID_HANDLE;
    command.m_shaderHandle = gameGraphicsData.m_shaderHandle;
    Tinker::Platform::InitDescSetDataHandles(command.m_descriptors);
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    command.m_numIndices = 3;
    command.m_numUVs = 0;
    command.m_numVertices = 3;
    command.m_indexBufferHandle = gameGraphicsData.m_indexBufferHandle2;
    command.m_vertexBufferHandle = gameGraphicsData.m_vertexBufferHandle2;
    command.m_uvBufferHandle = TINKER_INVALID_HANDLE;
    command.m_shaderHandle = gameGraphicsData.m_shaderHandle;
    Tinker::Platform::InitDescSetDataHandles(command.m_descriptors);
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassEnd;
    graphicsCommands.push_back(command);

    /*command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdImageCopy;
    command.m_srcImgHandle = gameGraphicsData.m_imageHandle;
    command.m_dstImgHandle = TINKER_INVALID_HANDLE;
    command.m_width = windowWidth;
    command.m_height = windowHeight;
    graphicsCommands.push_back(command);*/

    // Blit to screen
    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassBegin;
    command.m_renderPassHandle = TINKER_INVALID_HANDLE;
    command.m_framebufferHandle = TINKER_INVALID_HANDLE;
    command.m_renderWidth = 0;
    command.m_renderHeight = 0;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdDrawCall;
    command.m_numIndices = DEFAULT_QUAD_NUM_INDICES;
    command.m_numUVs = 0;
    command.m_numVertices = DEFAULT_QUAD_NUM_VERTICES;
    command.m_vertexBufferHandle = defaultQuad.m_vertexBufferHandle;
    command.m_indexBufferHandle = defaultQuad.m_indexBufferHandle;
    command.m_uvBufferHandle = TINKER_INVALID_HANDLE;
    command.m_shaderHandle = gameGraphicsData.m_blitShaderHandle;
    Tinker::Platform::InitDescSetDataHandles(command.m_descriptors);
    command.m_descriptors[0].handles[0] = gameGraphicsData.m_swapChainBlitDescHandle;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Tinker::Platform::eGraphicsCmdRenderPassEnd;
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
            // Sent message successfully
        }
    }

    return 0;
}

void DestroyWindowResizeDependentResources(const Tinker::Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyRenderPass(gameGraphicsData.m_mainRenderPassHandle);
    platformFuncs->DestroyFramebuffer(gameGraphicsData.m_framebufferHandle);
    platformFuncs->DestroyImageResource(gameGraphicsData.m_imageHandle);
    platformFuncs->DestroyImageViewResource(gameGraphicsData.m_imageViewHandle);

    DestroyShaders(platformFuncs);
    DestroyDescriptors(platformFuncs);
}

extern "C"
GAME_WINDOW_RESIZE(GameWindowResize)
{
    currentWindowWidth = newWindowWidth;
    currentWindowHeight = newWindowHeight;
    DestroyWindowResizeDependentResources(platformFuncs);

    CreateGameRenderingResources(platformFuncs, newWindowWidth, newWindowHeight);
    CreateAllDescriptors(platformFuncs);
    LoadAllShaders(platformFuncs, newWindowWidth, newWindowHeight);
}

extern "C"
GAME_DESTROY(GameDestroy)
{
    if (isGameInitted)
    {
        DestroyWindowResizeDependentResources(platformFuncs);

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

        if (isMultiplayer && connectedToServer)
        {
            platformFuncs->EndNetworkConnection();
        }
    }
}
