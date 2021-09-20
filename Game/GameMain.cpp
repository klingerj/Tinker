#include "Platform/PlatformGameAPI.h"
#include "Core/Allocators.h"
#include "Core/Math/VectorTypes.h"
#include "Core/FileIO/FileLoading.h"
#include "Core/Utility/ScopedTimer.h"
#include "GraphicsTypes.h"
#include "RenderPass.h"
#include "AssetManager.h"
#include "Camera.h"
#include "Raytracing.h"
#include "View.h"
#include "InputManager.h"
#include "Core/Graphics/VirtualTexture.h"

#include <string.h>

using namespace Tk;
using namespace Core;
using namespace Math;

static bool isGameInitted = false;
static const bool isMultiplayer = false;
static bool connectedToServer = false;
uint32 currentWindowWidth = 0, currentWindowHeight = 0;

static GameGraphicsData gameGraphicsData = {};
static GameRenderPass gameRenderPasses[eRenderPass_Max] = {};

static Camera g_gameCamera = {};
static const float cameraPanSensitivity = 0.1f;
static const float cameraRotSensitivityHorz = 0.001f;
static const float cameraRotSensitivityVert = 0.002f;
INPUT_CALLBACK(GameCameraPanForwardCallback)
{
    PanCameraAlongForward(&g_gameCamera, cameraPanSensitivity * param);
}
INPUT_CALLBACK(GameCameraPanBackwardCallback)
{
    PanCameraAlongForward(&g_gameCamera, -cameraPanSensitivity * param);
}
INPUT_CALLBACK(GameCameraPanRightCallback)
{
    PanCameraAlongRight(&g_gameCamera, cameraPanSensitivity * param);
}
INPUT_CALLBACK(GameCameraPanLeftCallback)
{
    PanCameraAlongRight(&g_gameCamera, -cameraPanSensitivity * param);
}
INPUT_CALLBACK(GameCameraRotateHorizontalCallback)
{
    RotateCameraAboutUp(&g_gameCamera, cameraRotSensitivityHorz * -(int32)param);
}
INPUT_CALLBACK(GameCameraRotateVerticalCallback)
{
    RotateCameraAboutRight(&g_gameCamera, cameraRotSensitivityVert * -(int32)param);
}

#define MAX_INSTANCES_PER_VIEW 128
static View MainView;

INPUT_CALLBACK(RaytraceTestCallback)
{
    Platform::PrintDebugString("Running raytrace test...\n");
    RaytraceTest(platformFuncs);
    Platform::PrintDebugString("...Done.\n");
}

void DestroyDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyDescriptor(gameGraphicsData.m_swapChainBlitDescHandle);
    gameGraphicsData.m_swapChainBlitDescHandle = Platform::DefaultDescHandle_Invalid;

    platformFuncs->DestroyDescriptor(gameGraphicsData.m_DescData_Instance);
    gameGraphicsData.m_DescData_Instance = Platform::DefaultDescHandle_Invalid;
    platformFuncs->DestroyResource(gameGraphicsData.m_DescDataBufferHandle_Instance);
    gameGraphicsData.m_DescDataBufferHandle_Instance = Platform::DefaultResHandle_Invalid;

    platformFuncs->DestroyDescriptor(gameGraphicsData.m_DescData_Global);
    gameGraphicsData.m_DescData_Global = Platform::DefaultDescHandle_Invalid;
    platformFuncs->DestroyResource(gameGraphicsData.m_DescDataBufferHandle_Global);
    gameGraphicsData.m_DescDataBufferHandle_Global = Platform::DefaultResHandle_Invalid;

    platformFuncs->DestroyAllDescriptors(); // destroys descriptor pool
}

void WriteSwapChainBlitResources(const Platform::PlatformAPIFuncs* platformFuncs)
{
    Platform::DescriptorSetDataHandles blitHandles = {};
    blitHandles.InitInvalid();
    blitHandles.handles[0] = gameGraphicsData.m_rtColorHandle;
    platformFuncs->WriteDescriptor(Tk::Platform::SHADER_ID_SWAP_CHAIN_BLIT, &gameGraphicsData.m_swapChainBlitDescHandle, 1, &blitHandles, 1);

    Platform::DescriptorSetDataHandles vbHandles = {};
    vbHandles.InitInvalid();
    vbHandles.handles[0] = defaultQuad.m_positionBuffer.gpuBufferHandle;
    vbHandles.handles[1] = defaultQuad.m_uvBuffer.gpuBufferHandle;
    vbHandles.handles[2] = defaultQuad.m_normalBuffer.gpuBufferHandle;
    platformFuncs->WriteDescriptor(Tk::Platform::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_VBS, &defaultQuad.m_descriptor, 1, &vbHandles, 1);
}

void CreateAllDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    // Swap chain blit
    gameGraphicsData.m_swapChainBlitDescHandle = platformFuncs->CreateDescriptor(Tk::Platform::SHADER_ID_SWAP_CHAIN_BLIT);
    WriteSwapChainBlitResources(platformFuncs);

    // Descriptor data
    Platform::ResourceDesc desc;
    desc.resourceType = Platform::ResourceType::eBuffer1D;
    desc.dims = v3ui(sizeof(DescriptorData_Instance) * MAX_INSTANCES_PER_VIEW, 0, 0);
    desc.bufferUsage = Platform::BufferUsage::eUniform;
    gameGraphicsData.m_DescDataBufferHandle_Instance = platformFuncs->CreateResource(desc);
    desc.dims = v3ui(sizeof(DescriptorData_Global), 0, 0);
    gameGraphicsData.m_DescDataBufferHandle_Global = platformFuncs->CreateResource(desc);

    gameGraphicsData.m_DescData_Global = platformFuncs->CreateDescriptor(Tk::Platform::DESCLAYOUT_ID_VIEW_GLOBAL);
    Platform::DescriptorHandle descHandles[MAX_BINDINGS_PER_SET] = { gameGraphicsData.m_DescData_Global, Platform::DefaultDescHandle_Invalid, Platform::DefaultDescHandle_Invalid };

    Platform::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descDataHandles[i].InitInvalid();
    descDataHandles[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Global;
    platformFuncs->WriteDescriptor(Tk::Platform::DESCLAYOUT_ID_VIEW_GLOBAL, &descHandles[0], 1, &descDataHandles[0], 1);

    gameGraphicsData.m_DescData_Instance = platformFuncs->CreateDescriptor(Tk::Platform::DESCLAYOUT_ID_ASSET_INSTANCE);
    descHandles[0] = gameGraphicsData.m_DescData_Instance;
    descHandles[1] = Platform::DefaultDescHandle_Invalid;
    descHandles[2] = Platform::DefaultDescHandle_Invalid;;

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descDataHandles[i].InitInvalid();
    descDataHandles[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Instance;
    platformFuncs->WriteDescriptor(Tk::Platform::DESCLAYOUT_ID_ASSET_INSTANCE, &descHandles[0], 1, &descDataHandles[0], 1);
}

void CreateGameRenderingResources(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    Platform::ResourceDesc desc;
    desc.resourceType = Platform::ResourceType::eImage2D;
    desc.dims = v3ui(windowWidth, windowHeight, 1);
    desc.imageFormat = Platform::ImageFormat::RGBA8_SRGB;
    gameGraphicsData.m_rtColorHandle = platformFuncs->CreateResource(desc);
    desc.imageFormat = Platform::ImageFormat::Depth_32F;
    gameGraphicsData.m_rtDepthHandle = platformFuncs->CreateResource(desc);

    // Depth-only pass
    gameGraphicsData.m_framebufferHandles[eRenderPass_ZPrePass] = platformFuncs->CreateFramebuffer(nullptr, 0, gameGraphicsData.m_rtDepthHandle, windowWidth, windowHeight, Platform::RENDERPASS_ID_ZPrepass);

    // Color and depth
    gameGraphicsData.m_framebufferHandles[eRenderPass_MainView] = platformFuncs->CreateFramebuffer(&gameGraphicsData.m_rtColorHandle, 1, gameGraphicsData.m_rtDepthHandle, windowWidth, windowHeight, Platform::RENDERPASS_ID_MainView);

    gameRenderPasses[eRenderPass_ZPrePass] = {};
    gameRenderPasses[eRenderPass_ZPrePass].framebuffer = gameGraphicsData.m_framebufferHandles[eRenderPass_ZPrePass];
    gameRenderPasses[eRenderPass_ZPrePass].renderPassID = Platform::RENDERPASS_ID_ZPrepass;
    gameRenderPasses[eRenderPass_ZPrePass].renderWidth = windowWidth;
    gameRenderPasses[eRenderPass_ZPrePass].renderHeight = windowHeight;
    gameRenderPasses[eRenderPass_ZPrePass].debugLabel = "Z Prepass";

    gameRenderPasses[eRenderPass_MainView] = {};
    gameRenderPasses[eRenderPass_MainView].framebuffer = gameGraphicsData.m_framebufferHandles[eRenderPass_MainView];
    gameRenderPasses[eRenderPass_MainView].renderPassID = Platform::RENDERPASS_ID_MainView;
    gameRenderPasses[eRenderPass_MainView].renderWidth = windowWidth;
    gameRenderPasses[eRenderPass_MainView].renderHeight = windowHeight;
    gameRenderPasses[eRenderPass_MainView].debugLabel = "Main Render View";
}

uint32 GameInit(const Tk::Platform::PlatformAPIFuncs* platformFuncs, Tk::Platform::GraphicsCommandStream* graphicsCommandStream, uint32 windowWidth, uint32 windowHeight)
{
    TIMED_SCOPED_BLOCK("Game Init");

    // Camera controls
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eW, GameCameraPanForwardCallback);
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eA, GameCameraPanLeftCallback);
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eS, GameCameraPanBackwardCallback);
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eD, GameCameraPanRightCallback);
    g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eW, GameCameraPanForwardCallback);
    g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eA, GameCameraPanLeftCallback);
    g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eS, GameCameraPanBackwardCallback);
    g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eD, GameCameraPanRightCallback);
    g_InputManager.BindMousecodeCallback(Platform::Mousecode::eMouseMoveHorizontal, GameCameraRotateHorizontalCallback);
    g_InputManager.BindMousecodeCallback(Platform::Mousecode::eMouseMoveVertical, GameCameraRotateVerticalCallback);

    // Hotkeys
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eF9, RaytraceTestCallback);

    g_gameCamera.m_ref = v3f(0.0f, 0.0f, 0.0f);
    g_gameCamera.m_eye = v3f(27.0f, 27.0f, 27.0f);
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
    DescriptorData_Instance data;
    data.modelMatrix = m4f(1.0f);

    Init(&MainView, MAX_INSTANCES_PER_VIEW);
    uint32 instanceID;
    instanceID = CreateInstance(&MainView, 0);
    data.modelMatrix[3][0] = -8.0f;
    SetInstanceData(&MainView, instanceID, &data);

    instanceID = CreateInstance(&MainView, 1);
    data.modelMatrix[3][0] = -2.5f;
    SetInstanceData(&MainView, instanceID, &data);

    instanceID = CreateInstance(&MainView, 2);
    data.modelMatrix = m4f(0.5f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][0] = 8.0f;
    SetInstanceData(&MainView, instanceID, &data);
    
    instanceID = CreateInstance(&MainView, 2);
    data.modelMatrix = m4f(0.25f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][0] = 8.0f;
    data.modelMatrix[3][2] = 6.0f;
    SetInstanceData(&MainView, instanceID, &data);

    instanceID = CreateInstance(&MainView, 3);
    data.modelMatrix = m4f(7.0f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][1] = 8.0f;
    SetInstanceData(&MainView, instanceID, &data);
    
    instanceID = CreateInstance(&MainView, 3);
    data.modelMatrix = m4f(7.0f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][1] = 10.0f;
    SetInstanceData(&MainView, instanceID, &data);

    CreateAnimatedPoly(platformFuncs, &gameGraphicsData.m_animatedPolygon);

    CreateAllDescriptors(platformFuncs);

    // TODO: TEMP: test virtual texture
    Core::Graphics::VirtualTexture vt;
    vt.Reset();
    vt.Create(platformFuncs, 4, 16, v2ui(1024, 1024), v2ui(1024, 1024));

    return 0;
}

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

    UpdateAxisVectors(&g_gameCamera);

    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;

    {
        //TIMED_SCOPED_BLOCK("Input manager update - kb/mouse callbacks");
        g_InputManager.UpdateAndDoCallbacks(inputStateDeltas, platformFuncs);
    }

    // Update view(s)
    {
        //TIMED_SCOPED_BLOCK("View update");
        MainView.m_viewMatrix = CameraViewMatrix(&g_gameCamera);
        MainView.m_projMatrix = g_projMat;

        Platform::DescriptorSetDataHandles descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0].InitInvalid();
        descriptors[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Global;
        descriptors[1].InitInvalid();
        descriptors[1].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Instance;
        Update(&MainView, descriptors, platformFuncs);
    }

    // Clear depth buffer
    {
        Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

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
        //TIMED_SCOPED_BLOCK("Record render pass commands");

        Platform::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0] = gameGraphicsData.m_DescData_Global;
        descriptors[1] = gameGraphicsData.m_DescData_Instance;

        StartRenderPass(&gameRenderPasses[eRenderPass_ZPrePass], graphicsCommandStream);
        RecordRenderPassCommands(&MainView, &gameRenderPasses[eRenderPass_ZPrePass], graphicsCommandStream, Tk::Platform::SHADER_ID_BASIC_ZPrepass, Platform::BlendState::eNoColorAttachment, Platform::DepthState::eTestOnWriteOn, descriptors);
        EndRenderPass(&gameRenderPasses[eRenderPass_ZPrePass], graphicsCommandStream);

        StartRenderPass(&gameRenderPasses[eRenderPass_MainView], graphicsCommandStream);
        RecordRenderPassCommands(&MainView, &gameRenderPasses[eRenderPass_MainView], graphicsCommandStream, Tk::Platform::SHADER_ID_BASIC_MainView, Platform::BlendState::eAlphaBlend, Platform::DepthState::eTestOnWriteOn, descriptors);

        UpdateAnimatedPoly(platformFuncs, &gameGraphicsData.m_animatedPolygon);
        DrawAnimatedPoly(&gameGraphicsData.m_animatedPolygon, gameGraphicsData.m_DescData_Global, Tk::Platform::SHADER_ID_ANIMATEDPOLY_MainView, Platform::BlendState::eAlphaBlend, Platform::DepthState::eTestOnWriteOn, graphicsCommandStream);
        EndRenderPass(&gameRenderPasses[eRenderPass_MainView], graphicsCommandStream);
    }

    // FINAL BLIT TO SCREEN
    Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Platform::GraphicsCmd::eRenderPassBegin;
    command->debugLabel = "Blit to screen";
    command->m_framebufferHandle = Platform::DefaultFramebufferHandle_Invalid;
    command->m_renderPassID = Platform::RENDERPASS_ID_SWAP_CHAIN_BLIT;
    command->m_renderWidth = 0;
    command->m_renderHeight = 0;
    ++graphicsCommandStream->m_numCommands;
    ++command;

    command->m_commandType = Platform::GraphicsCmd::eDrawCall;
    command->debugLabel = "Draw default quad";
    command->m_numIndices = DEFAULT_QUAD_NUM_INDICES;
    command->m_numInstances = 1;
    command->m_indexBufferHandle = defaultQuad.m_indexBuffer.gpuBufferHandle;
    command->m_shader = Tk::Platform::SHADER_ID_SWAP_CHAIN_BLIT;
    command->m_blendState = Platform::BlendState::eReplace;
    command->m_depthState = Platform::DepthState::eOff;
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        command->m_descriptors[i] = Platform::DefaultDescHandle_Invalid;
    }
    command->m_descriptors[0] = gameGraphicsData.m_swapChainBlitDescHandle;
    command->m_descriptors[1] = defaultQuad.m_descriptor;
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
    WriteSwapChainBlitResources(platformFuncs);
}

extern "C"
GAME_DESTROY(GameDestroy)
{
    if (isGameInitted)
    {
        DestroyWindowResizeDependentResources(platformFuncs);
        DestroyDescriptors(platformFuncs);

        DestroyDefaultGeometry(platformFuncs);
        DestroyDefaultGeometryVertexBufferDescriptor(defaultQuad, platformFuncs);
        
        DestroyAnimatedPoly(platformFuncs, &gameGraphicsData.m_animatedPolygon);

        // Destroy assets
        g_AssetManager.DestroyAllMeshData(platformFuncs);
        g_AssetManager.DestroyAllTextureData(platformFuncs);

        if (isMultiplayer && connectedToServer)
        {
            platformFuncs->EndNetworkConnection();
        }

        g_AssetManager.FreeMemory();
    }

    #if defined(MEM_TRACKING)
    _CrtDumpMemoryLeaks();
    #endif
}
