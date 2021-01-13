#include "../Include/PlatformGameAPI.h"
#include "../Include/Core/Allocators.h"
#include "../Include/Core/Math/VectorTypes.h"
#include "../Include/Core/Containers/RingBuffer.h"
#include "../Include/Core/FileIO/FileLoading.h"
#include "GameGraphicsTypes.h"
#include "AssetManager.h"
#include "Camera.h"

#include <cstring>
#include <string.h>
#include <vector>

using namespace Tinker;
using namespace Core;
using namespace Math;

/*static void Test_Thread_Func()
{
    Platform::PrintDebugString("I am from a thread.\n");
}
*/

#define DEFAULT_QUAD_NUM_VERTICES 4
#define DEFAULT_QUAD_NUM_INDICES 6
DefaultGeometry<DEFAULT_QUAD_NUM_VERTICES, DEFAULT_QUAD_NUM_INDICES> defaultQuad = {
    // buffer handles
    { DefaultResHandle_Invalid, DefaultResHandle_Invalid, nullptr },
    { DefaultResHandle_Invalid, DefaultResHandle_Invalid, nullptr },
    { DefaultResHandle_Invalid, DefaultResHandle_Invalid, nullptr },
    { DefaultResHandle_Invalid, DefaultResHandle_Invalid, nullptr },
    // positions
    v4f(-1.0f, -1.0f, 0.0f, 1.0f),
    v4f(1.0f, -1.0f, 0.0f, 1.0f),
    v4f(-1.0f, 1.0f, 0.0f, 1.0f),
    v4f(1.0f, 1.0f, 0.0f, 1.0f),
    //uvs
    v2f(0.0f, 0.0f),
    v2f(0.0f, 1.0f),
    v2f(0.0f, 1.0f),
    v2f(1.0f, 1.0f),
    // normals
    v3f(0.0f, 0.0f, 1.0f),
    v3f(0.0f, 0.0f, 1.0f),
    v3f(0.0f, 0.0f, 1.0f),
    v3f(0.0f, 0.0f, 1.0f),
    // indices
    0, 1, 2, 2, 1, 3
};

static GameGraphicsData gameGraphicsData = {};

static std::vector<Platform::GraphicsCommand> graphicsCommands;

static bool isGameInitted = false;
static const bool isMultiplayer = false;
static bool connectedToServer = false;

uint32 currentWindowWidth, currentWindowHeight;

Memory::LinearAllocator shaderBytecodeAllocator;
const uint32 totalShaderBytecodeMaxSizeInBytes = 1024 * 10;

typedef struct input_state
{
    Platform::KeycodeState keyCodes[Platform::eMaxKeycodes];
} InputState;
static InputState currentInputState = {};
static InputState previousInputState = {};

typedef struct descriptor_instance_data
{
    alignas(16) m4f modelMatrix;
    alignas(16) m4f viewProj;
} DescriptorInstanceData;

static VirtualCamera g_gameCamera = {};

// TODO: remove me
#include <chrono>
void UpdateDescriptorState()
{
    // TODO: remove this, is just for testing
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    float scale = 1.0f;
    DescriptorInstanceData instanceData = {};
    instanceData.modelMatrix = m4f(scale);
    //instanceData.modelMatrix[1][1] = 1.0f;
    instanceData.modelMatrix[3][3] = 1.0f;
    instanceData.viewProj = g_projMat * CameraViewMatrix(&g_gameCamera);
    memcpy(gameGraphicsData.m_modelMatrixBufferMemPtr1, &instanceData, sizeof(instanceData));
}

Platform::ResourceHandle LoadShader(const Platform::PlatformAPIFuncs* platformFuncs, const char* vertexShaderFileName, const char* fragmentShaderFileName, Platform::GraphicsPipelineParams* params)
{
    // get file size, load entire file
    uint32 vertexShaderFileSize = platformFuncs->GetFileSize(vertexShaderFileName);
    uint32 fragmentShaderFileSize = platformFuncs->GetFileSize(fragmentShaderFileName);
    TINKER_ASSERT(vertexShaderFileSize > 0);
    TINKER_ASSERT(fragmentShaderFileSize > 0);

    uint8* vertexShaderBuffer = shaderBytecodeAllocator.Alloc(vertexShaderFileSize, 1);
    uint8* fragmentShaderBuffer = shaderBytecodeAllocator.Alloc(fragmentShaderFileSize, 1);
    
    platformFuncs->ReadEntireFile(vertexShaderFileName, vertexShaderFileSize, vertexShaderBuffer);
    platformFuncs->ReadEntireFile(fragmentShaderFileName, fragmentShaderFileSize, fragmentShaderBuffer);

    return platformFuncs->CreateGraphicsPipeline(vertexShaderBuffer, vertexShaderFileSize, fragmentShaderBuffer, fragmentShaderFileSize, params->blendState, params->depthState, params->viewportWidth, params->viewportHeight, params->renderPassHandle, params->descriptorHandle);
}

void LoadAllShaders(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    shaderBytecodeAllocator.Free();
    shaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);

    Platform::DescriptorLayout mainDrawDescriptorLayout = {};
    Platform::InitDescLayout(&mainDrawDescriptorLayout);
    Platform::GraphicsPipelineParams params;
    params.blendState = Platform::eBlendStateAlphaBlend;
    params.depthState = Platform::eDepthStateTestOnWriteOn;
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.renderPassHandle = gameGraphicsData.m_mainRenderPassHandle;
    params.descriptorHandle = gameGraphicsData.m_modelMatrixDescHandle1;
    gameGraphicsData.m_shaderHandle = LoadShader(platformFuncs, "..\\Shaders\\spv\\basic_vert_glsl.spv", "..\\Shaders\\spv\\basic_frag_glsl.spv", &params);

    params.blendState = Platform::eBlendStateAlphaBlend;
    params.depthState = Platform::eDepthStateOff;
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.renderPassHandle = DefaultResHandle_Invalid;
    params.descriptorHandle = gameGraphicsData.m_swapChainBlitDescHandle;
    gameGraphicsData.m_blitShaderHandle = LoadShader(platformFuncs, "..\\Shaders\\spv\\blit_vert_glsl.spv", "..\\Shaders\\spv\\blit_frag_glsl.spv", &params);
}

void DestroyShaders(const Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_shaderHandle);
    platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_blitShaderHandle);
    gameGraphicsData.m_shaderHandle = DefaultResHandle_Invalid;
    gameGraphicsData.m_blitShaderHandle = DefaultResHandle_Invalid;
}

void DestroyDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyDescriptor(gameGraphicsData.m_swapChainBlitDescHandle);
    gameGraphicsData.m_swapChainBlitDescHandle = DefaultDescHandle_Invalid;

    platformFuncs->DestroyDescriptor(gameGraphicsData.m_modelMatrixDescHandle1);
    gameGraphicsData.m_modelMatrixDescHandle1 = DefaultDescHandle_Invalid;
    platformFuncs->UnmapResource(gameGraphicsData.m_modelMatrixBufferHandle1);
    platformFuncs->DestroyResource(gameGraphicsData.m_modelMatrixBufferHandle1);
    gameGraphicsData.m_modelMatrixBufferHandle1 = DefaultResHandle_Invalid;

    platformFuncs->DestroyAllDescriptors(); // TODO: this is not a good API and should be per-pool or something
}

void CreateAllDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    // Swap chain blit
    Platform::DescriptorLayout blitDescriptorLayout = {};
    Platform::InitDescLayout(&blitDescriptorLayout);
    blitDescriptorLayout.descriptorTypes[0][0].type = Platform::eDescriptorTypeSampledImage;
    blitDescriptorLayout.descriptorTypes[0][0].amount = 1;
    gameGraphicsData.m_swapChainBlitDescHandle = platformFuncs->CreateDescriptor(&blitDescriptorLayout);

    Platform::DescriptorSetDataHandles blitHandles = {};
    blitHandles.handles[0] = gameGraphicsData.m_imageHandle;
    platformFuncs->WriteDescriptor(&blitDescriptorLayout, gameGraphicsData.m_swapChainBlitDescHandle, &blitHandles);

    // Model matrix
    ResourceDesc desc;
    desc.resourceType = Platform::eResourceTypeBuffer1D;
    desc.dims = v3ui(sizeof(DescriptorInstanceData), 0, 0);
    desc.bufferUsage = Platform::eBufferUsageUniform;
    gameGraphicsData.m_modelMatrixBufferHandle1 = platformFuncs->CreateResource(desc);
    gameGraphicsData.m_modelMatrixBufferMemPtr1 = platformFuncs->MapResource(gameGraphicsData.m_modelMatrixBufferHandle1);

    Platform::DescriptorLayout instanceDataDescriptorLayout = {};
    Platform::InitDescLayout(&instanceDataDescriptorLayout);
    instanceDataDescriptorLayout.descriptorTypes[0][0].type = Platform::eDescriptorTypeBuffer;
    instanceDataDescriptorLayout.descriptorTypes[0][0].amount = 1;
    gameGraphicsData.m_modelMatrixDescHandle1 = platformFuncs->CreateDescriptor(&instanceDataDescriptorLayout);

    Platform::DescriptorSetDataHandles modeMatrixHandles = {};
    modeMatrixHandles.handles[0] = gameGraphicsData.m_modelMatrixBufferHandle1;
    platformFuncs->WriteDescriptor(&instanceDataDescriptorLayout, gameGraphicsData.m_modelMatrixDescHandle1, &modeMatrixHandles);
}

void RecreateShaders(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    DestroyShaders(platformFuncs);
    
    DestroyDescriptors(platformFuncs);

    CreateAllDescriptors(platformFuncs);
    LoadAllShaders(platformFuncs, windowWidth, windowHeight);
}

void CreateGameRenderingResources(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    ResourceDesc desc;
    desc.resourceType = Platform::eResourceTypeImage2D;
    desc.dims = v3ui(windowWidth, windowHeight, 1);
    gameGraphicsData.m_imageHandle = platformFuncs->CreateResource(desc);
    gameGraphicsData.m_mainRenderPassHandle = platformFuncs->CreateRenderPass(Platform::eImageLayoutUndefined, Platform::eImageLayoutShaderRead);
    gameGraphicsData.m_framebufferHandle = platformFuncs->CreateFramebuffer(&gameGraphicsData.m_imageHandle, 1, windowWidth, windowHeight, gameGraphicsData.m_mainRenderPassHandle);
}

void ProcessInputState(const Platform::InputStateDeltas* inputStateDeltas, const Platform::PlatformAPIFuncs* platformFuncs)
{
    previousInputState = currentInputState;

    for (uint32 uiKeycode = 0; uiKeycode < Platform::eMaxKeycodes; ++uiKeycode)
    {
        if (inputStateDeltas->keyCodes[uiKeycode].numStateChanges > 0)
        {
            currentInputState.keyCodes[uiKeycode].isDown = inputStateDeltas->keyCodes[uiKeycode].isDown;
            currentInputState.keyCodes[uiKeycode].numStateChanges += inputStateDeltas->keyCodes[uiKeycode].numStateChanges;
            // TODO: reset number of state changes at some point, every second or every several frames or something
        }
    }

    for (uint32 uiKeycode = 0; uiKeycode < Platform::eMaxKeycodes; ++uiKeycode)
    {
        switch (uiKeycode)
        {
            case Platform::eKeyF10:
            {
                // Handle the initial downpress once and nothing more
                if (currentInputState.keyCodes[uiKeycode].isDown && !previousInputState.keyCodes[uiKeycode].isDown)
                {
                    Platform::PrintDebugString("Attempting to hotload shaders...\n");

                    // Recompile shaders via script
                    const char* shaderCompileCommand = "..\\Scripts\\compile_shaders_glsl2spv.bat";
                    if (platformFuncs->SystemCommand(shaderCompileCommand) != 0)
                    {
                        Platform::PrintDebugString("Failed to create shader compile process! Shaders will not be compiled.\n");
                    }
                    else
                    {
                        // Recreate gpu resources
                        RecreateShaders(platformFuncs, currentWindowWidth, currentWindowHeight);
                        Platform::PrintDebugString("...Done.\n");
                    }
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
        g_gameCamera.m_ref = v3f(0.0f, 0.0f, 0.0f);
        currentWindowWidth = windowWidth;
        currentWindowHeight = windowHeight;
        g_projMat = PerspectiveProjectionMatrix((float)currentWindowWidth / currentWindowHeight);

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

        // Load mesh files
        g_AssetManager.LoadAllAssets(platformFuncs);
        g_AssetManager.InitAssetGraphicsResources(platformFuncs);

        graphicsCommands.reserve(graphicsCommandStream->m_maxCommands);

        // Default geometry
        ResourceDesc desc;
        desc.resourceType = Platform::eResourceTypeBuffer1D;

        desc.dims = v3ui(sizeof(defaultQuad.m_points), 0, 0);
        desc.bufferUsage = Platform::eBufferUsageVertex;
        defaultQuad.m_positionBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::eBufferUsageStaging;
        defaultQuad.m_positionBuffer.stagingBufferHandle = platformFuncs->CreateResource(desc);
        defaultQuad.m_positionBuffer.stagingBufferMemPtr = platformFuncs->MapResource(defaultQuad.m_positionBuffer.stagingBufferHandle);
        memcpy(defaultQuad.m_positionBuffer.stagingBufferMemPtr, defaultQuad.m_points, desc.dims.x);

        desc.dims = v3ui(sizeof(defaultQuad.m_uvs), 0, 0);
        desc.bufferUsage = Platform::eBufferUsageVertex;
        defaultQuad.m_uvBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::eBufferUsageStaging;
        defaultQuad.m_uvBuffer.stagingBufferHandle = platformFuncs->CreateResource(desc);
        defaultQuad.m_uvBuffer.stagingBufferMemPtr = platformFuncs->MapResource(defaultQuad.m_uvBuffer.stagingBufferHandle);
        memcpy(defaultQuad.m_uvBuffer.stagingBufferMemPtr, defaultQuad.m_uvs, desc.dims.x);

        desc.dims = v3ui(sizeof(defaultQuad.m_normals), 0, 0);
        desc.bufferUsage = Platform::eBufferUsageVertex;
        defaultQuad.m_normalBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::eBufferUsageStaging;
        defaultQuad.m_normalBuffer.stagingBufferHandle = platformFuncs->CreateResource(desc);
        defaultQuad.m_normalBuffer.stagingBufferMemPtr = platformFuncs->MapResource(defaultQuad.m_normalBuffer.stagingBufferHandle);
        memcpy(defaultQuad.m_normalBuffer.stagingBufferMemPtr, defaultQuad.m_normals, desc.dims.x);

        desc.dims = v3ui(sizeof(defaultQuad.m_indices), 0, 0);
        desc.bufferUsage = Platform::eBufferUsageIndex;
        defaultQuad.m_indexBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::eBufferUsageStaging;
        defaultQuad.m_indexBuffer.stagingBufferHandle = platformFuncs->CreateResource(desc);
        defaultQuad.m_indexBuffer.stagingBufferMemPtr = platformFuncs->MapResource(defaultQuad.m_indexBuffer.stagingBufferHandle);
        memcpy(defaultQuad.m_indexBuffer.stagingBufferMemPtr, defaultQuad.m_indices, desc.dims.x);

        Platform::GraphicsCommand command = {};
        command.m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;
        command.m_sizeInBytes = sizeof(defaultQuad.m_points);
        command.m_dstBufferHandle = defaultQuad.m_positionBuffer.gpuBufferHandle;
        command.m_srcBufferHandle = defaultQuad.m_positionBuffer.stagingBufferHandle;
        graphicsCommands.push_back(command);
        command.m_sizeInBytes = sizeof(defaultQuad.m_indices);
        command.m_dstBufferHandle = defaultQuad.m_indexBuffer.gpuBufferHandle;
        command.m_srcBufferHandle = defaultQuad.m_indexBuffer.stagingBufferHandle;
        graphicsCommands.push_back(command);
        command.m_sizeInBytes = sizeof(defaultQuad.m_normals);
        command.m_dstBufferHandle = defaultQuad.m_normalBuffer.gpuBufferHandle;
        command.m_srcBufferHandle = defaultQuad.m_normalBuffer.stagingBufferHandle;
        graphicsCommands.push_back(command);

        CreateGameRenderingResources(platformFuncs, windowWidth, windowHeight);

        CreateAllDescriptors(platformFuncs);
        LoadAllShaders(platformFuncs, windowWidth, windowHeight);

        isGameInitted = true;
    }

    // TODO: move this
    // TODO: pass time data into this function
    // Animate camera
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    g_gameCamera.m_eye = v3f(cosf(time) * 27.0f, sinf(time) * 27.0f, 27.0f);

    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;

    ProcessInputState(inputStateDeltas, platformFuncs);
    UpdateDescriptorState();

    // Test a thread job
    /*Platform::WorkerJob* jobs[32] = {};
    for (uint32 i = 0; i < 32; ++i)
    {
        jobs[i] = Platform::CreateNewThreadJob([=]()
                {
                    // TODO: custom string library...
                    char* string = "I am from a thread";
                    char dst[50] = "";
                    strcpy_s(dst, string);
                    _itoa_s(i, dst + strlen(string), 10, 10);
                    if (i < 10) dst[strlen(string) + 1] = ' ';
                    dst[strlen(string) + 2] = '\n';
                    dst[strlen(string) + 3] = '\0';
                    Platform::PrintDebugString(dst);
                });
        platformFuncs->EnqueueWorkerThreadJob(jobs[i]);
    }
    for (uint32 i = 0; i < 32; ++i)
    {
        platformFuncs->WaitOnThreadJob(jobs[i]);
    }*/
    
    // Issue graphics commands
    Platform::GraphicsCommand command = {};

    // Record buffer update commands
    for (uint32 uiAssetID = 0; uiAssetID < g_AssetManager.m_numMeshAssets; ++uiAssetID)
    {
        DynamicMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(uiAssetID);

        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_positionBuffer, meshData->m_numIndices * sizeof(v4f), "Update Asset Vtx Pos Buf");
        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_uvBuffer, meshData->m_numIndices * sizeof(v2f), "Update Asset Vtx Uv Buf");
        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_normalBuffer, meshData->m_numIndices * sizeof(v3f), "Update Asset Vtx Norm Buf");
        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_indexBuffer, meshData->m_numIndices * sizeof(uint32), "Update Asset Vtx Idx Buf");
    }

    command.m_commandType = (uint32)Platform::eGraphicsCmdRenderPassBegin;
    command.debugLabel = "Main Draw Pass";
    command.m_renderPassHandle = gameGraphicsData.m_mainRenderPassHandle;
    command.m_framebufferHandle = gameGraphicsData.m_framebufferHandle;
    command.m_renderWidth = windowWidth;
    command.m_renderHeight = windowHeight;
    graphicsCommands.push_back(command);

    // Draw calls
    Platform::DescriptorSetDescHandles descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
    InitDescSetDescHandles(descriptors);
    descriptors[0].handles[0] = gameGraphicsData.m_modelMatrixDescHandle1;

    for (uint32 uiAssetID = 0; uiAssetID < g_AssetManager.m_numMeshAssets; ++uiAssetID)
    {
        DynamicMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(uiAssetID);

        DrawMeshDataCommand(graphicsCommands,
            meshData->m_numIndices,
            meshData->m_indexBuffer.gpuBufferHandle,
            meshData->m_positionBuffer.gpuBufferHandle,
            meshData->m_uvBuffer.gpuBufferHandle,
            meshData->m_normalBuffer.gpuBufferHandle,
            gameGraphicsData.m_shaderHandle,
            descriptors,
            "Draw asset");
    }

    command.m_commandType = (uint32)Platform::eGraphicsCmdRenderPassEnd;
    graphicsCommands.push_back(command);

    /*command.m_commandType = (uint32)Platform::eGraphicsCmdImageCopy;
    command.m_srcImgHandle = gameGraphicsData.m_imageHandle;
    command.m_dstImgHandle = DefaultResHandle_Invalid;
    command.m_width = windowWidth;
    command.m_height = windowHeight;
    graphicsCommands.push_back(command);*/

    // Blit to screen
    command.m_commandType = (uint32)Platform::eGraphicsCmdRenderPassBegin;
    command.debugLabel = "Blit to screen";
    command.m_renderPassHandle = DefaultResHandle_Invalid;
    command.m_framebufferHandle = DefaultResHandle_Invalid;
    command.m_renderWidth = 0;
    command.m_renderHeight = 0;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Platform::eGraphicsCmdDrawCall;
    command.debugLabel = "Draw default quad";
    command.m_numIndices = DEFAULT_QUAD_NUM_INDICES;
    command.m_positionBufferHandle = defaultQuad.m_positionBuffer.gpuBufferHandle;
    command.m_uvBufferHandle = defaultQuad.m_uvBuffer.gpuBufferHandle;
    command.m_normalBufferHandle = defaultQuad.m_normalBuffer.gpuBufferHandle;
    command.m_indexBufferHandle = defaultQuad.m_indexBuffer.gpuBufferHandle;
    command.m_shaderHandle = gameGraphicsData.m_blitShaderHandle;
    Platform::InitDescSetDescHandles(command.m_descriptors);
    command.m_descriptors[0].handles[0] = gameGraphicsData.m_swapChainBlitDescHandle;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Platform::eGraphicsCmdRenderPassEnd;
    graphicsCommands.push_back(command);

    graphicsCommandStream->m_numCommands = (uint32)graphicsCommands.size();
    Platform::GraphicsCommand* graphicsCmdBase = graphicsCommandStream->m_graphicsCommands;
    uint32 graphicsCmdSizeInBytes = (uint32)graphicsCommands.size() * sizeof(Platform::GraphicsCommand);
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

void DestroyWindowResizeDependentResources(const Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyRenderPass(gameGraphicsData.m_mainRenderPassHandle);
    platformFuncs->DestroyFramebuffer(gameGraphicsData.m_framebufferHandle);
    platformFuncs->DestroyResource(gameGraphicsData.m_imageHandle);

    DestroyShaders(platformFuncs);
    DestroyDescriptors(platformFuncs);
}

extern "C"
GAME_WINDOW_RESIZE(GameWindowResize)
{
    currentWindowWidth = newWindowWidth;
    currentWindowHeight = newWindowHeight;
    DestroyWindowResizeDependentResources(platformFuncs);

    // Gameplay stuff
    g_projMat = PerspectiveProjectionMatrix((float)currentWindowWidth / currentWindowHeight);

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
        platformFuncs->DestroyResource(defaultQuad.m_positionBuffer.gpuBufferHandle);
        platformFuncs->UnmapResource(defaultQuad.m_positionBuffer.stagingBufferHandle);
        platformFuncs->DestroyResource(defaultQuad.m_positionBuffer.stagingBufferHandle);

        platformFuncs->DestroyResource(defaultQuad.m_uvBuffer.gpuBufferHandle);
        platformFuncs->UnmapResource(defaultQuad.m_uvBuffer.stagingBufferHandle);
        platformFuncs->DestroyResource(defaultQuad.m_uvBuffer.stagingBufferHandle);

        platformFuncs->DestroyResource(defaultQuad.m_normalBuffer.gpuBufferHandle);
        platformFuncs->UnmapResource(defaultQuad.m_normalBuffer.stagingBufferHandle);
        platformFuncs->DestroyResource(defaultQuad.m_normalBuffer.stagingBufferHandle);

        platformFuncs->DestroyResource(defaultQuad.m_indexBuffer.gpuBufferHandle);
        platformFuncs->UnmapResource(defaultQuad.m_indexBuffer.stagingBufferHandle);
        platformFuncs->DestroyResource(defaultQuad.m_indexBuffer.stagingBufferHandle);
        //-----

        // Game graphics
        for (uint32 uiAssetID = 0; uiAssetID < g_AssetManager.m_numMeshAssets; ++uiAssetID)
        {
            DynamicMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(uiAssetID);

            platformFuncs->DestroyResource(meshData->m_positionBuffer.gpuBufferHandle);
            platformFuncs->UnmapResource(meshData->m_positionBuffer.stagingBufferHandle);
            platformFuncs->DestroyResource(meshData->m_positionBuffer.stagingBufferHandle);

            platformFuncs->DestroyResource(meshData->m_uvBuffer.gpuBufferHandle);
            platformFuncs->UnmapResource(meshData->m_uvBuffer.stagingBufferHandle);
            platformFuncs->DestroyResource(meshData->m_uvBuffer.stagingBufferHandle);

            platformFuncs->DestroyResource(meshData->m_normalBuffer.gpuBufferHandle);
            platformFuncs->UnmapResource(meshData->m_normalBuffer.stagingBufferHandle);
            platformFuncs->DestroyResource(meshData->m_normalBuffer.stagingBufferHandle);

            platformFuncs->DestroyResource(meshData->m_indexBuffer.gpuBufferHandle);
            platformFuncs->UnmapResource(meshData->m_indexBuffer.stagingBufferHandle);
            platformFuncs->DestroyResource(meshData->m_indexBuffer.stagingBufferHandle);
        }

        if (isMultiplayer && connectedToServer)
        {
            platformFuncs->EndNetworkConnection();
        }
    }
}
