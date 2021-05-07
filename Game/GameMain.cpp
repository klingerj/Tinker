#include "PlatformGameAPI.h"
#include "Core/Allocators.h"
#include "Core/Math/VectorTypes.h"
#include "Core/FileIO/FileLoading.h"
#include "Core/Utilities/ScopedTimer.h"
#include "GameGraphicsTypes.h"
#include "ShaderLoading.h"
#include "GameRenderPass.h"
#include "AssetManager.h"
#include "Camera.h"
#include "GameRaytracing.h"
#include "View.h"

#ifdef _SCRIPTS_DIR
#define SCRIPTS_PATH STRINGIFY(_SCRIPTS_DIR)
#else
//#define SCRIPTS_PATH "..\\Scripts\\"
#endif

#include <cstring>
#include <string.h>

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
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
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
static GameRenderPass gameRenderPasses[eRenderPass_Max] = {};

static bool isGameInitted = false;
static const bool isMultiplayer = false;
static bool connectedToServer = false;

uint32 currentWindowWidth, currentWindowHeight;

typedef struct input_state
{
    Platform::KeycodeState keyCodes[Platform::Keycode::eMax];
} InputState;
static InputState currentInputState = {};
static InputState previousInputState = {};

static VirtualCamera g_gameCamera = {};

static View MainView;

void LoadAllShaders(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    ResetShaderBytecodeAllocator();

    Platform::DescriptorLayout mainDrawDescriptorLayout = {};
    mainDrawDescriptorLayout.InitInvalid();
    Platform::GraphicsPipelineParams params;
    params.blendState = Platform::BlendState::eInvalid; // no color attachment
    params.depthState = Platform::DepthState::eTestOnWriteOn; // Write to depth
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.framebufferHandle = gameGraphicsData.m_framebufferHandles[eRenderPass_ZPrePass];

    DescriptorHandle descHandles[2] = { gameGraphicsData.m_DescData_Global, gameGraphicsData.m_DescData_Instance};
    params.descriptorHandles = descHandles;
    params.numDescriptorHandles = 2;
    gameGraphicsData.m_shaderHandles[eRenderPass_ZPrePass] = LoadShader(platformFuncs, SHADERS_SPV_PATH "basic_vert_glsl.spv", nullptr, &params);

    params.framebufferHandle = gameGraphicsData.m_framebufferHandles[eRenderPass_MainView];
    params.blendState = Platform::BlendState::eAlphaBlend; // Default alpha blending for now
    params.depthState = Platform::DepthState::eTestOnWriteOff; // Read depth buffer, don't write to it
    gameGraphicsData.m_shaderHandles[eRenderPass_MainView] = LoadShader(platformFuncs, SHADERS_SPV_PATH "basic_vert_glsl.spv", SHADERS_SPV_PATH "basic_frag_glsl.spv", &params);

    gameRenderPasses[eRenderPass_ZPrePass].shader = gameGraphicsData.m_shaderHandles[eRenderPass_ZPrePass];
    gameRenderPasses[eRenderPass_MainView].shader = gameGraphicsData.m_shaderHandles[eRenderPass_MainView];

    params.blendState = Platform::BlendState::eAlphaBlend;
    params.depthState = Platform::DepthState::eOff;
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.framebufferHandle = DefaultFramebufferHandle_Invalid;
    params.descriptorHandles = &gameGraphicsData.m_swapChainBlitDescHandle;
    params.numDescriptorHandles = 1;
    gameGraphicsData.m_blitShaderHandle = LoadShader(platformFuncs, SHADERS_SPV_PATH "blit_vert_glsl.spv", SHADERS_SPV_PATH "blit_frag_glsl.spv", &params);
}

void DestroyShaders(const Platform::PlatformAPIFuncs* platformFuncs)
{
    for (uint32 uiPass = 0; uiPass < eRenderPass_Max; ++uiPass)
    {
        platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_shaderHandles[uiPass]);
        gameGraphicsData.m_shaderHandles[uiPass] = DefaultShaderHandle_Invalid;
    }

    platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_blitShaderHandle);
    gameGraphicsData.m_blitShaderHandle = DefaultShaderHandle_Invalid;
}

void DestroyDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyDescriptor(gameGraphicsData.m_swapChainBlitDescHandle);
    gameGraphicsData.m_swapChainBlitDescHandle = DefaultDescHandle_Invalid;

    platformFuncs->DestroyDescriptor(gameGraphicsData.m_DescData_Instance);
    gameGraphicsData.m_DescData_Instance = DefaultDescHandle_Invalid;
    platformFuncs->DestroyResource(gameGraphicsData.m_DescDataBufferHandle_Instance);
    gameGraphicsData.m_DescDataBufferHandle_Instance = DefaultResHandle_Invalid;

    platformFuncs->DestroyDescriptor(gameGraphicsData.m_DescData_Global);
    gameGraphicsData.m_DescData_Global = DefaultDescHandle_Invalid;
    platformFuncs->DestroyResource(gameGraphicsData.m_DescDataBufferHandle_Global);
    gameGraphicsData.m_DescDataBufferHandle_Global = DefaultResHandle_Invalid;

    platformFuncs->DestroyAllDescriptors(); // TODO: this is not a good API and should be per-pool or something
}

void CreateAllDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    // Swap chain blit
    Platform::DescriptorLayout blitDescriptorLayout = {};
    blitDescriptorLayout.InitInvalid();
    blitDescriptorLayout.descriptorLayoutParams[0][0].type = Platform::DescriptorType::eSampledImage;
    blitDescriptorLayout.descriptorLayoutParams[0][0].amount = 1;
    gameGraphicsData.m_swapChainBlitDescHandle = platformFuncs->CreateDescriptor(&blitDescriptorLayout);

    Platform::DescriptorSetDataHandles blitHandles = {};
    blitHandles.InitInvalid();
    blitHandles.handles[0] = gameGraphicsData.m_rtColorHandle;
    platformFuncs->WriteDescriptor(&blitDescriptorLayout, &gameGraphicsData.m_swapChainBlitDescHandle, &blitHandles);

    // Descriptor data
    ResourceDesc desc;
    desc.resourceType = Platform::ResourceType::eBuffer1D;
    desc.dims = v3ui(sizeof(DescriptorData_Instance), 0, 0);
    desc.bufferUsage = Platform::BufferUsage::eUniform;
    gameGraphicsData.m_DescDataBufferHandle_Instance = platformFuncs->CreateResource(desc);
    desc.dims = v3ui(sizeof(DescriptorData_Global), 0, 0);
    gameGraphicsData.m_DescDataBufferHandle_Global = platformFuncs->CreateResource(desc);

    Platform::DescriptorLayout DescriptorLayout = {};
    DescriptorLayout.InitInvalid();
    DescriptorLayout.descriptorLayoutParams[0][0].type = Platform::DescriptorType::eBuffer;
    DescriptorLayout.descriptorLayoutParams[0][0].amount = 1;
    // TODO: add the number of vertex attribute buffers here ^^

    DescriptorLayout.descriptorLayoutParams[1][0].type = Platform::DescriptorType::eBuffer;
    DescriptorLayout.descriptorLayoutParams[1][0].amount = 1;

    gameGraphicsData.m_DescData_Global = platformFuncs->CreateDescriptor(&DescriptorLayout);
    gameGraphicsData.m_DescData_Instance = platformFuncs->CreateDescriptor(&DescriptorLayout);

    Platform::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    descDataHandles[0].InitInvalid();
    descDataHandles[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Global;
    descDataHandles[1].InitInvalid();
    descDataHandles[1].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Instance;

    DescriptorHandle descHandles[2] = { gameGraphicsData.m_DescData_Global, gameGraphicsData.m_DescData_Instance };
    platformFuncs->WriteDescriptor(&DescriptorLayout, &descHandles[0], &descDataHandles[0]);
    platformFuncs->WriteDescriptor(&DescriptorLayout, &descHandles[1], &descDataHandles[1]);
}

void RecreateShaders(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    DestroyShaders(platformFuncs);
    DestroyDescriptors(platformFuncs);

    CreateAllDescriptors(platformFuncs);
    LoadAllShaders(platformFuncs, windowWidth, windowHeight);
}

void CreateDefaultGeometry(const Platform::PlatformAPIFuncs* platformFuncs, Platform::GraphicsCommandStream* graphicsCommandStream)
{
    // Default Quad
    {
        ResourceDesc desc;
        desc.resourceType = Platform::ResourceType::eBuffer1D;

        // Positions
        desc.dims = v3ui(sizeof(defaultQuad.m_points), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eVertex;
        defaultQuad.m_positionBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_Pos = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_Pos = platformFuncs->MapResource(stagingBufferHandle_Pos);

        // UVs
        desc.dims = v3ui(sizeof(defaultQuad.m_uvs), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eVertex;
        defaultQuad.m_uvBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_UV = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_UV = platformFuncs->MapResource(stagingBufferHandle_UV);

        // Normals
        desc.dims = v3ui(sizeof(defaultQuad.m_normals), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eVertex;
        defaultQuad.m_normalBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_Norm = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_Norm = platformFuncs->MapResource(stagingBufferHandle_Norm);

        // Indices
        desc.dims = v3ui(sizeof(defaultQuad.m_indices), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eIndex;
        defaultQuad.m_indexBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_Idx = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_Idx = platformFuncs->MapResource(stagingBufferHandle_Idx);

        // Memcpy into staging buffers
        memcpy(stagingBufferMemPtr_Pos, defaultQuad.m_points, sizeof(defaultQuad.m_points));
        memcpy(stagingBufferMemPtr_UV, defaultQuad.m_uvs, sizeof(defaultQuad.m_uvs));
        memcpy(stagingBufferMemPtr_Norm, defaultQuad.m_normals, sizeof(defaultQuad.m_normals));
        memcpy(stagingBufferMemPtr_Idx, defaultQuad.m_indices, sizeof(defaultQuad.m_indices));

        // Do GPU buffer copies
        Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];
        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx Pos Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_points);
        command->m_dstBufferHandle = defaultQuad.m_positionBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_Pos;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx UV Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_uvs);
        command->m_dstBufferHandle = defaultQuad.m_uvBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_UV;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx Norm Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_normals);
        command->m_dstBufferHandle = defaultQuad.m_normalBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_Norm;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx Idx Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_indices);
        command->m_dstBufferHandle = defaultQuad.m_indexBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_Idx;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        platformFuncs->SubmitCmdsImmediate(graphicsCommandStream);
        graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

        // Unmap + destroy resources
        platformFuncs->UnmapResource(stagingBufferHandle_Pos);
        platformFuncs->UnmapResource(stagingBufferHandle_UV);
        platformFuncs->UnmapResource(stagingBufferHandle_Norm);
        platformFuncs->UnmapResource(stagingBufferHandle_Idx);

        platformFuncs->DestroyResource(stagingBufferHandle_Pos);
        platformFuncs->DestroyResource(stagingBufferHandle_UV);
        platformFuncs->DestroyResource(stagingBufferHandle_Norm);
        platformFuncs->DestroyResource(stagingBufferHandle_Idx);
    }
}

void DestroyDefaultGeometry(const Platform::PlatformAPIFuncs* platformFuncs)
{
    // Default quad
    platformFuncs->DestroyResource(defaultQuad.m_positionBuffer.gpuBufferHandle);
    platformFuncs->DestroyResource(defaultQuad.m_uvBuffer.gpuBufferHandle);
    platformFuncs->DestroyResource(defaultQuad.m_normalBuffer.gpuBufferHandle);
    platformFuncs->DestroyResource(defaultQuad.m_indexBuffer.gpuBufferHandle);
}

void CreateGameRenderingResources(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    ResourceDesc desc;
    desc.resourceType = Platform::ResourceType::eImage2D;
    desc.dims = v3ui(windowWidth, windowHeight, 1);
    desc.imageFormat = Platform::ImageFormat::RGBA8_SRGB;
    gameGraphicsData.m_rtColorHandle = platformFuncs->CreateResource(desc);
    desc.imageFormat = Platform::ImageFormat::Depth_32F;
    gameGraphicsData.m_rtDepthHandle = platformFuncs->CreateResource(desc);

    // Depth-only pass
    gameGraphicsData.m_framebufferHandles[eRenderPass_ZPrePass] = platformFuncs->CreateFramebuffer(nullptr, 0, gameGraphicsData.m_rtDepthHandle, Platform::ImageLayout::eUndefined, windowWidth, windowHeight);

    // Color and depth
    gameGraphicsData.m_framebufferHandles[eRenderPass_MainView] = platformFuncs->CreateFramebuffer(&gameGraphicsData.m_rtColorHandle, 1, gameGraphicsData.m_rtDepthHandle, Platform::ImageLayout::eShaderRead, windowWidth, windowHeight);

    gameRenderPasses[eRenderPass_ZPrePass] = {};
    gameRenderPasses[eRenderPass_ZPrePass].framebuffer = gameGraphicsData.m_framebufferHandles[eRenderPass_ZPrePass];
    gameRenderPasses[eRenderPass_ZPrePass].renderWidth = windowWidth;
    gameRenderPasses[eRenderPass_ZPrePass].renderHeight = windowHeight;
    gameRenderPasses[eRenderPass_ZPrePass].debugLabel = "Z Prepass";

    gameRenderPasses[eRenderPass_MainView] = {};
    gameRenderPasses[eRenderPass_MainView].framebuffer = gameGraphicsData.m_framebufferHandles[eRenderPass_MainView];
    gameRenderPasses[eRenderPass_MainView].renderWidth = windowWidth;
    gameRenderPasses[eRenderPass_MainView].renderHeight = windowHeight;
    gameRenderPasses[eRenderPass_MainView].debugLabel = "Main Render View";
}

void ProcessInputState(const Platform::InputStateDeltas* inputStateDeltas, const Platform::PlatformAPIFuncs* platformFuncs)
{
    previousInputState = currentInputState;

    for (uint32 uiKeycode = 0; uiKeycode < Platform::Keycode::eMax; ++uiKeycode)
    {
        if (inputStateDeltas->keyCodes[uiKeycode].numStateChanges > 0)
        {
            currentInputState.keyCodes[uiKeycode].isDown = inputStateDeltas->keyCodes[uiKeycode].isDown;
            currentInputState.keyCodes[uiKeycode].numStateChanges += inputStateDeltas->keyCodes[uiKeycode].numStateChanges;
            // TODO: reset number of state changes at some point, every second or every several frames or something
        }
    }

    for (uint32 uiKeycode = 0; uiKeycode < Platform::Keycode::eMax; ++uiKeycode)
    {
        switch (uiKeycode)
        {
            case Platform::Keycode::eF10:
            {
                // Handle the initial downpress once and nothing more
                if (currentInputState.keyCodes[uiKeycode].isDown && !previousInputState.keyCodes[uiKeycode].isDown)
                {
                    Platform::PrintDebugString("Attempting to hotload shaders...\n");

                    // Recompile shaders via script
                    const char* shaderCompileCommand = SCRIPTS_PATH "build_compile_shaders_glsl2spv.bat";
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

uint32 GameInit(const Tinker::Platform::PlatformAPIFuncs* platformFuncs, Tinker::Platform::GraphicsCommandStream* graphicsCommandStream, uint32 windowWidth, uint32 windowHeight)
{
    TIMED_SCOPED_BLOCK("Game Init");

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

    {
        TIMED_SCOPED_BLOCK("Load game assets");
        g_AssetManager.LoadAllAssets(platformFuncs);
        g_AssetManager.InitAssetGraphicsResources(platformFuncs, graphicsCommandStream);
    }

    CreateDefaultGeometry(platformFuncs, graphicsCommandStream);

    CreateGameRenderingResources(platformFuncs, windowWidth, windowHeight);

    // Init view(s)
    Init(&MainView);
    uint32 instanceID;
    instanceID = CreateInstance(&MainView, 0);
    instanceID = CreateInstance(&MainView, 1);
    instanceID = CreateInstance(&MainView, 2);
    instanceID = CreateInstance(&MainView, 3);

    CreateAllDescriptors(platformFuncs);
    LoadAllShaders(platformFuncs, windowWidth, windowHeight);

    if (0)
    {
        RaytraceTest(platformFuncs);
    }

    return 0;
}

// TODO: remove me
#include <chrono>
extern "C"
GAME_UPDATE(GameUpdate)
{
    graphicsCommandStream->m_numCommands = 0;

    if (!isGameInitted)
    {
        uint32 initResult = GameInit(platformFuncs, graphicsCommandStream, windowWidth, windowHeight);
        if (initResult != 0)
        {
            return initResult;
        }
        isGameInitted = true;
    }

    // TODO: move this
    // Animate camera
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    g_gameCamera.m_eye = v3f(cosf(time) * 27.0f, sinf(time) * 27.0f, 27.0f);

    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;

    ProcessInputState(inputStateDeltas, platformFuncs);

    // Update view(s)
    {
        MainView.m_viewMatrix = CameraViewMatrix(&g_gameCamera);
        MainView.m_projMatrix = g_projMat;

        Platform::DescriptorSetDataHandles descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0].InitInvalid();
        descriptors[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Global;
        descriptors[1].InitInvalid();
        descriptors[1].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Instance;
        Update(&MainView, descriptors, platformFuncs);
    }

    // Record buffer update commands
    // TODO: have dynamic assets?
    /*for (uint32 uiAssetID = 0; uiAssetID < g_AssetManager.m_numMeshAssets; ++uiAssetID)
    {
        DynamicMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(uiAssetID);

        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_positionBuffer, meshData->m_numIndices * sizeof(v4f), "Update Asset Vtx Pos Buf");
        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_uvBuffer, meshData->m_numIndices * sizeof(v2f), "Update Asset Vtx Uv Buf");
        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_normalBuffer, meshData->m_numIndices * sizeof(v3f), "Update Asset Vtx Norm Buf");
        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_indexBuffer, meshData->m_numIndices * sizeof(uint32), "Update Asset Vtx Idx Buf");
    }*/

    // Clear depth buffer
    {
        Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

        // Transition of depth buffer from layout undefined to transfer_dst (required for clear command)
        command->m_commandType = Platform::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition depth to transfer_dst";
        command->m_imageHandle = gameGraphicsData.m_rtDepthHandle;
        command->m_startLayout = Platform::ImageLayout::eUndefined;
        command->m_endLayout = Platform::ImageLayout::eTransferDst;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Clear depth buffer - before z-prepass
        command->m_commandType = Platform::GraphicsCmd::eClearImage;
        command->debugLabel = "Clear depth buffer";
        command->m_imageHandle = gameGraphicsData.m_rtDepthHandle;
        command->m_clearValue = v4f(1.0f, 0.0f, 0.0f, 0.0f); // depth/stencil clear uses x and y components
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Transition of depth buffer from transfer dst to depth_attachment_optimal
        command->m_commandType = Platform::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition depth to depth_attachment_optimal";
        command->m_imageHandle = gameGraphicsData.m_rtDepthHandle;
        command->m_startLayout = Platform::ImageLayout::eTransferDst;
        command->m_endLayout = Platform::ImageLayout::eDepthOptimal;
        ++graphicsCommandStream->m_numCommands;
        ++command;
    }

    // Record render commands for view(s)
    {
        Platform::DescriptorSetDescHandles descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0].InitInvalid();
        descriptors[0].handles[0] = gameGraphicsData.m_DescData_Global;
        descriptors[1].InitInvalid();
        descriptors[1].handles[0] = gameGraphicsData.m_DescData_Instance;
        RecordRenderPassCommands(&MainView, &gameRenderPasses[eRenderPass_ZPrePass], graphicsCommandStream, descriptors);
        RecordRenderPassCommands(&MainView, &gameRenderPasses[eRenderPass_MainView], graphicsCommandStream, descriptors);
    }

    // FINAL BLIT TO SCREEN
    Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    // Blit to screen
    command->m_commandType = Platform::GraphicsCmd::eRenderPassBegin;
    command->debugLabel = "Blit to screen";
    command->m_framebufferHandle = DefaultFramebufferHandle_Invalid;
    command->m_renderWidth = 0;
    command->m_renderHeight = 0;
    ++graphicsCommandStream->m_numCommands;
    ++command;

    command->m_commandType = Platform::GraphicsCmd::eDrawCall;
    command->debugLabel = "Draw default quad";
    command->m_numIndices = DEFAULT_QUAD_NUM_INDICES;
    command->m_positionBufferHandle = defaultQuad.m_positionBuffer.gpuBufferHandle;
    command->m_uvBufferHandle = defaultQuad.m_uvBuffer.gpuBufferHandle;
    command->m_normalBufferHandle = defaultQuad.m_normalBuffer.gpuBufferHandle;
    command->m_indexBufferHandle = defaultQuad.m_indexBuffer.gpuBufferHandle;
    command->m_shaderHandle = gameGraphicsData.m_blitShaderHandle;
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        command->m_descriptors[i].InitInvalid();
    }
    command->m_descriptors[0].handles[0] = gameGraphicsData.m_swapChainBlitDescHandle;
    ++graphicsCommandStream->m_numCommands;
    ++command;

    command->m_commandType = Platform::GraphicsCmd::eRenderPassEnd;
    ++graphicsCommandStream->m_numCommands;
    ++command;

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
    for (uint32 uiPass = 0; uiPass < eRenderPass_Max; ++uiPass)
    {
        platformFuncs->DestroyFramebuffer(gameGraphicsData.m_framebufferHandles[uiPass]);
    }
    platformFuncs->DestroyResource(gameGraphicsData.m_rtColorHandle);
    platformFuncs->DestroyResource(gameGraphicsData.m_rtDepthHandle);

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

        DestroyDefaultGeometry(platformFuncs);

        // Game graphics
        for (uint32 uiAssetID = 0; uiAssetID < g_AssetManager.m_numMeshAssets; ++uiAssetID)
        {
            StaticMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(uiAssetID);

            platformFuncs->DestroyResource(meshData->m_positionBuffer.gpuBufferHandle);
            platformFuncs->DestroyResource(meshData->m_uvBuffer.gpuBufferHandle);
            platformFuncs->DestroyResource(meshData->m_normalBuffer.gpuBufferHandle);
            platformFuncs->DestroyResource(meshData->m_indexBuffer.gpuBufferHandle);
        }

        for (uint32 uiAssetID = 0; uiAssetID < g_AssetManager.m_numTextureAssets; ++uiAssetID)
        {
            ResourceHandle textureHandle = g_AssetManager.GetTextureGraphicsDataByID(uiAssetID);
            platformFuncs->DestroyResource(textureHandle);
        }

        if (isMultiplayer && connectedToServer)
        {
            platformFuncs->EndNetworkConnection();
        }

        g_AssetManager.FreeMemory();
        FreeShaderBytecodeMemory();
    }

    #if defined(MEM_TRACKING)
    _CrtDumpMemoryLeaks();
    #endif
}
