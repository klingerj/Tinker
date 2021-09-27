#include "Platform/PlatformGameAPI.h"
#include "Platform/Graphics/GraphicsCommon.h"
#include "Allocators.h"
#include "Math/VectorTypes.h"
#include "FileLoading.h"
#include "Utility/ScopedTimer.h"
#include "GraphicsTypes.h"
#include "RenderPass.h"
#include "AssetManager.h"
#include "Camera.h"
#include "Raytracing.h"
#include "View.h"
#include "InputManager.h"

//#include "Core/Graphics/VirtualTexture.h"

#include <string.h>

using namespace Tk;
using namespace Platform;
using namespace Core;

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

//Core::Graphics::VirtualTexture vt;

#define MAX_INSTANCES_PER_VIEW 128
static View MainView;

INPUT_CALLBACK(RaytraceTestCallback)
{
    Platform::PrintDebugString("Running raytrace test...\n");
    RaytraceTest(platformFuncs);
    Platform::PrintDebugString("...Done.\n");
}

void DestroyDescriptors()
{
    Graphics::DestroyDescriptor(gameGraphicsData.m_swapChainBlitDescHandle);
    gameGraphicsData.m_swapChainBlitDescHandle = Graphics::DefaultDescHandle_Invalid;

    Graphics::DestroyDescriptor(gameGraphicsData.m_DescData_Instance);
    gameGraphicsData.m_DescData_Instance = Graphics::DefaultDescHandle_Invalid;
    Graphics::DestroyResource(gameGraphicsData.m_DescDataBufferHandle_Instance);
    gameGraphicsData.m_DescDataBufferHandle_Instance = Graphics::DefaultResHandle_Invalid;

    Graphics::DestroyDescriptor(gameGraphicsData.m_DescData_Global);
    gameGraphicsData.m_DescData_Global = Graphics::DefaultDescHandle_Invalid;
    Graphics::DestroyResource(gameGraphicsData.m_DescDataBufferHandle_Global);
    gameGraphicsData.m_DescDataBufferHandle_Global = Graphics::DefaultResHandle_Invalid;

    Graphics::DestroyAllDescriptors(); // destroys descriptor pool
}

void WriteSwapChainBlitResources()
{
    Graphics::DescriptorSetDataHandles blitHandles = {};
    blitHandles.InitInvalid();
    blitHandles.handles[0] = gameGraphicsData.m_rtColorHandle;
    Graphics::WriteDescriptor(Graphics::SHADER_ID_SWAP_CHAIN_BLIT, &gameGraphicsData.m_swapChainBlitDescHandle, 1, &blitHandles, 1);

    Graphics::DescriptorSetDataHandles vbHandles = {};
    vbHandles.InitInvalid();
    vbHandles.handles[0] = defaultQuad.m_positionBuffer.gpuBufferHandle;
    vbHandles.handles[1] = defaultQuad.m_uvBuffer.gpuBufferHandle;
    vbHandles.handles[2] = defaultQuad.m_normalBuffer.gpuBufferHandle;
    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_VBS, &defaultQuad.m_descriptor, 1, &vbHandles, 1);
}

void CreateAllDescriptors()
{
    // Swap chain blit
    gameGraphicsData.m_swapChainBlitDescHandle = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_TEX);
    WriteSwapChainBlitResources();

    // Descriptor data
    Graphics::ResourceDesc desc;
    desc.resourceType = Graphics::ResourceType::eBuffer1D;
    desc.dims = v3ui(sizeof(DescriptorData_Instance) * MAX_INSTANCES_PER_VIEW, 0, 0);
    desc.bufferUsage = Graphics::BufferUsage::eUniform;
    gameGraphicsData.m_DescDataBufferHandle_Instance = Graphics::CreateResource(desc);
    desc.dims = v3ui(sizeof(DescriptorData_Global), 0, 0);
    gameGraphicsData.m_DescDataBufferHandle_Global = Graphics::CreateResource(desc);

    gameGraphicsData.m_DescData_Global = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_VIEW_GLOBAL);
    Graphics::DescriptorHandle descHandles[MAX_BINDINGS_PER_SET] = { gameGraphicsData.m_DescData_Global, Graphics::DefaultDescHandle_Invalid, Graphics::DefaultDescHandle_Invalid };

    Graphics::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descDataHandles[i].InitInvalid();
    descDataHandles[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Global;
    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_VIEW_GLOBAL, &descHandles[0], 1, &descDataHandles[0], 1);

    gameGraphicsData.m_DescData_Instance = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_ASSET_INSTANCE);
    descHandles[0] = gameGraphicsData.m_DescData_Instance;
    descHandles[1] = Graphics::DefaultDescHandle_Invalid;
    descHandles[2] = Graphics::DefaultDescHandle_Invalid;

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descDataHandles[i].InitInvalid();
    descDataHandles[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Instance;
    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_ASSET_INSTANCE, &descHandles[0], 1, &descDataHandles[0], 1);
}

void CreateGameRenderingResources(uint32 windowWidth, uint32 windowHeight)
{
    Graphics::ResourceDesc desc;
    desc.resourceType = Graphics::ResourceType::eImage2D;
    desc.arrayEles = 1;
    desc.dims = v3ui(windowWidth, windowHeight, 1);
    desc.imageFormat = Graphics::ImageFormat::RGBA8_SRGB;
    gameGraphicsData.m_rtColorHandle = Graphics::CreateResource(desc);
    desc.imageFormat = Graphics::ImageFormat::Depth_32F;
    gameGraphicsData.m_rtDepthHandle = Graphics::CreateResource(desc);

    // Depth-only pass
    gameGraphicsData.m_framebufferHandles[eRenderPass_ZPrePass] = Graphics::CreateFramebuffer(nullptr, 0, gameGraphicsData.m_rtDepthHandle, windowWidth, windowHeight, Graphics::RENDERPASS_ID_ZPrepass);

    // Color and depth
    gameGraphicsData.m_framebufferHandles[eRenderPass_MainView] = Graphics::CreateFramebuffer(&gameGraphicsData.m_rtColorHandle, 1, gameGraphicsData.m_rtDepthHandle, windowWidth, windowHeight, Graphics::RENDERPASS_ID_MainView);

    gameRenderPasses[eRenderPass_ZPrePass] = {};
    gameRenderPasses[eRenderPass_ZPrePass].framebuffer = gameGraphicsData.m_framebufferHandles[eRenderPass_ZPrePass];
    gameRenderPasses[eRenderPass_ZPrePass].renderPassID = Graphics::RENDERPASS_ID_ZPrepass;
    gameRenderPasses[eRenderPass_ZPrePass].renderWidth = windowWidth;
    gameRenderPasses[eRenderPass_ZPrePass].renderHeight = windowHeight;
    gameRenderPasses[eRenderPass_ZPrePass].debugLabel = "Z Prepass";

    gameRenderPasses[eRenderPass_MainView] = {};
    gameRenderPasses[eRenderPass_MainView].framebuffer = gameGraphicsData.m_framebufferHandles[eRenderPass_MainView];
    gameRenderPasses[eRenderPass_MainView].renderPassID = Graphics::RENDERPASS_ID_MainView;
    gameRenderPasses[eRenderPass_MainView].renderWidth = windowWidth;
    gameRenderPasses[eRenderPass_MainView].renderHeight = windowHeight;
    gameRenderPasses[eRenderPass_MainView].debugLabel = "Main Render View";
}

uint32 GameInit(const Tk::Platform::PlatformAPIFuncs* platformFuncs, Graphics::GraphicsCommandStream* graphicsCommandStream, uint32 windowWidth, uint32 windowHeight)
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
        g_AssetManager.InitAssetGraphicsResources(graphicsCommandStream);
    }

    CreateDefaultGeometry(graphicsCommandStream);

    CreateGameRenderingResources(windowWidth, windowHeight);

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

    CreateAnimatedPoly(&gameGraphicsData.m_animatedPolygon);

    CreateAllDescriptors();

    // TODO: TEMP: test virtual texture
    //vt.Reset();
    //vt.Create(graphicsCommandStream, 4, 16, v2ui(1024, 1024), v2ui(1024, 1024));

    return 0;
}

// TODO: move this out
/*void DrawTerrain(Tk::Platform::Graphics::GraphicsCommandStream* graphicsCommandStream)
{
    Tk::Platform::Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Graphics::GraphicsCmd::eDrawCall;
    command->debugLabel = "Draw terrain quad";
    command->m_numIndices = DEFAULT_QUAD_NUM_INDICES;
    command->m_numInstances = 1;
    command->m_indexBufferHandle = vt.m_terrainIdx;
    command->m_shader = Graphics::SHADER_ID_BASIC_VirtualTexture;
    command->m_blendState = Graphics::BlendState::eAlphaBlend;
    command->m_depthState = Graphics::DepthState::eTestOnWriteOn;
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        command->m_descriptors[i] = Graphics::DefaultDescHandle_Invalid;
    }
    command->m_descriptors[0] = gameGraphicsData.m_DescData_Global;
    command->m_descriptors[1] = vt.m_desc_terrainPos;
    command->m_descriptors[2] = vt.m_desc;
    command->m_descriptors[3] = vt.m_desc_terrain;
    ++graphicsCommandStream->m_numCommands;
}*/

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

        Graphics::DescriptorSetDataHandles descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0].InitInvalid();
        descriptors[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Global;
        descriptors[1].InitInvalid();
        descriptors[1].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Instance;
        Update(&MainView, descriptors);
    }

    //vt.Update(platformFuncs);

    // Clear depth buffer
    {
        Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

        // Transition of depth buffer from layout undefined to transfer_dst (required for clear command)
        command->m_commandType = Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition depth to transfer_dst";
        command->m_imageHandle = gameGraphicsData.m_rtDepthHandle;
        command->m_startLayout = Graphics::ImageLayout::eUndefined;
        command->m_endLayout = Graphics::ImageLayout::eTransferDst;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Clear depth buffer - before z-prepass
        command->m_commandType = Graphics::GraphicsCmd::eClearImage;
        command->debugLabel = "Clear depth buffer";
        command->m_imageHandle = gameGraphicsData.m_rtDepthHandle;
        command->m_clearValue = v4f(1.0f, 0.0f, 0.0f, 0.0f); // depth/stencil clear uses x and y components
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Transition of depth buffer from transfer dst to depth_attachment_optimal
        command->m_commandType = Graphics::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition depth to depth_attachment_optimal";
        command->m_imageHandle = gameGraphicsData.m_rtDepthHandle;
        command->m_startLayout = Graphics::ImageLayout::eTransferDst;
        command->m_endLayout = Graphics::ImageLayout::eDepthOptimal;
        ++graphicsCommandStream->m_numCommands;
        ++command;
    }

    // Record render commands for view(s)
    {
        //TIMED_SCOPED_BLOCK("Record render pass commands");

        Graphics::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0] = gameGraphicsData.m_DescData_Global;
        descriptors[1] = gameGraphicsData.m_DescData_Instance;

        StartRenderPass(&gameRenderPasses[eRenderPass_ZPrePass], graphicsCommandStream);
        RecordRenderPassCommands(&MainView, &gameRenderPasses[eRenderPass_ZPrePass], graphicsCommandStream, Graphics::SHADER_ID_BASIC_ZPrepass, Graphics::BlendState::eNoColorAttachment, Graphics::DepthState::eTestOnWriteOn, descriptors);
        EndRenderPass(&gameRenderPasses[eRenderPass_ZPrePass], graphicsCommandStream);

        StartRenderPass(&gameRenderPasses[eRenderPass_MainView], graphicsCommandStream);
        RecordRenderPassCommands(&MainView, &gameRenderPasses[eRenderPass_MainView], graphicsCommandStream, Graphics::SHADER_ID_BASIC_MainView, Graphics::BlendState::eAlphaBlend, Graphics::DepthState::eTestOnWriteOn, descriptors);

        UpdateAnimatedPoly(&gameGraphicsData.m_animatedPolygon);
        DrawAnimatedPoly(&gameGraphicsData.m_animatedPolygon, gameGraphicsData.m_DescData_Global, Graphics::SHADER_ID_ANIMATEDPOLY_MainView, Graphics::BlendState::eAlphaBlend, Graphics::DepthState::eTestOnWriteOn, graphicsCommandStream);

        //DrawTerrain(graphicsCommandStream);

        EndRenderPass(&gameRenderPasses[eRenderPass_MainView], graphicsCommandStream);
    }

    // FINAL BLIT TO SCREEN
    Graphics::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Graphics::GraphicsCmd::eRenderPassBegin;
    command->debugLabel = "Blit to screen";
    command->m_framebufferHandle = Graphics::DefaultFramebufferHandle_Invalid;
    command->m_renderPassID = Graphics::RENDERPASS_ID_SWAP_CHAIN_BLIT;
    command->m_renderWidth = 0;
    command->m_renderHeight = 0;
    ++graphicsCommandStream->m_numCommands;
    ++command;

    command->m_commandType = Graphics::GraphicsCmd::eDrawCall;
    command->debugLabel = "Draw default quad";
    command->m_numIndices = DEFAULT_QUAD_NUM_INDICES;
    command->m_numInstances = 1;
    command->m_indexBufferHandle = defaultQuad.m_indexBuffer.gpuBufferHandle;
    command->m_shader = Graphics::SHADER_ID_SWAP_CHAIN_BLIT;
    command->m_blendState = Graphics::BlendState::eReplace;
    command->m_depthState = Graphics::DepthState::eOff;
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        command->m_descriptors[i] = Graphics::DefaultDescHandle_Invalid;
    }
    command->m_descriptors[0] = gameGraphicsData.m_swapChainBlitDescHandle;
    command->m_descriptors[1] = defaultQuad.m_descriptor;
    ++graphicsCommandStream->m_numCommands;
    ++command;

    command->m_commandType = Graphics::GraphicsCmd::eRenderPassEnd;
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

void DestroyWindowResizeDependentResources()
{
    for (uint32 uiPass = 0; uiPass < eRenderPass_Max; ++uiPass)
    {
        Graphics::DestroyFramebuffer(gameGraphicsData.m_framebufferHandles[uiPass]);
    }
    Graphics::DestroyResource(gameGraphicsData.m_rtColorHandle);
    Graphics::DestroyResource(gameGraphicsData.m_rtDepthHandle);
}

extern "C"
GAME_WINDOW_RESIZE(GameWindowResize)
{
    currentWindowWidth = newWindowWidth;
    currentWindowHeight = newWindowHeight;
    DestroyWindowResizeDependentResources();

    // Gameplay stuff
    g_projMat = PerspectiveProjectionMatrix((float)currentWindowWidth / currentWindowHeight);

    CreateGameRenderingResources(newWindowWidth, newWindowHeight);
    WriteSwapChainBlitResources();
}

extern "C"
GAME_DESTROY(GameDestroy)
{
    if (isGameInitted)
    {
        DestroyWindowResizeDependentResources();
        DestroyDescriptors();

        DestroyDefaultGeometry();
        DestroyDefaultGeometryVertexBufferDescriptor(defaultQuad);
        
        DestroyAnimatedPoly(&gameGraphicsData.m_animatedPolygon);

        //vt.Destroy(platformFuncs);

        // Destroy assets
        g_AssetManager.DestroyAllMeshData();
        g_AssetManager.DestroyAllTextureData();

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
