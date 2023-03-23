#include "Platform/PlatformGameAPI.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "Graphics/Common/GPUTimestamps.h"
#include "Graphics/Common/ShaderManager.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "Allocators.h"
#include "Math/VectorTypes.h"
#include "AssetFileParsing.h"
#include "Utility/ScopedTimer.h"
#include "GraphicsTypes.h"
#include "RenderPasses/RenderPass.h"
#include "RenderPasses/ZPrepassRenderPass.h"
#include "RenderPasses/ForwardRenderPass.h"
#include "RenderPasses/SwapChainBlitRenderPass.h"
#include "AssetManager.h"
#include "Camera.h"
#include "Raytracing.h"
#include "View.h"
#include "Scene.h"
#include "InputManager.h"
#include "DebugUI.h"

// Unity style build
#include "GraphicsTypes.cpp"
#include "AssetManager.cpp"
#include "RenderPasses/RenderPass.cpp"
#include "RenderPasses/ZPrepassRenderPass.cpp"
#include "RenderPasses/ForwardRenderPass.cpp"
#include "RenderPasses/DebugUIRenderPass.cpp"
#include "RenderPasses/SwapChainBlitRenderPass.cpp"
#include "Raytracing.cpp"
#include "View.cpp"
#include "Scene.cpp"
#include "Camera.cpp"
#include "InputManager.cpp"
#include "DebugUI.cpp"
#include "imgui.cpp"
#include "imgui_demo.cpp"
#include "imgui_draw.cpp"
#include "imgui_tables.cpp"
#include "imgui_widgets.cpp"
//

#include <string.h>

using namespace Tk;
using namespace Platform;

static bool isGameInitted = false;
static const bool isMultiplayer = false;
static bool connectedToServer = false;
static uint32 currentWindowWidth = 0;
static uint32 currentWindowHeight = 0;
static bool isWindowMinimized;
static Tk::Platform::WindowHandles* windowHandles = nullptr;

#define TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX MAX_UINT16
Tk::Graphics::GraphicsCommandStream graphicsCommandStream;

// For now, this owns all RTs
GameGraphicsData gameGraphicsData = {};
static GameRenderPass gameRenderPassList[eRenderPass_Max] = {};

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

INPUT_CALLBACK(HotloadAllShaders)
{
    Tk::Core::Utility::LogMsg("Game", "Attempting to hotload shaders...\n", Tk::Core::Utility::LogSeverity::eInfo);

    uint32 result = Tk::ShaderCompiler::ErrCode::NonShaderError;
    #ifdef VULKAN
    result = Tk::ShaderCompiler::CompileAllShadersVK();
    #else
    #endif
    
    if (result == Tk::ShaderCompiler::ErrCode::Success)
    {
        Tk::Graphics::ShaderManager::ReloadShaders(currentWindowWidth, currentWindowHeight);
        Tk::Core::Utility::LogMsg("Game", "...Done.\n", Tk::Core::Utility::LogSeverity::eInfo);
    }
    else
    {
        // TODO: grab error message from shader compiler
        Tk::Core::Utility::LogMsg("Game", "Shader compilation failed.\n", Tk::Core::Utility::LogSeverity::eWarning);
    }
}

#define MAX_INSTANCES_PER_SCENE 128
extern Scene MainScene;
extern View MainView;

Scene MainScene = {};
View MainView = {};

INPUT_CALLBACK(RaytraceTestCallback)
{
    Platform::PrintDebugString("Running raytrace test...\n");
    RaytraceTest();
    Platform::PrintDebugString("...Done.\n");
}

static void InitDemo()
{
    // Init scene
    Init(&MainScene, MAX_INSTANCES_PER_SCENE, &g_InputManager);
    
    // Init view
    DescriptorData_Instance data;
    data.modelMatrix = m4f(1.0f);

    Init(&MainView);
    uint32 instanceID;
    instanceID = CreateInstance(&MainScene, 0);
    data.modelMatrix[3][0] = -8.0f;
    SetInstanceData(&MainScene, instanceID, &data);

    instanceID = CreateInstance(&MainScene, 1);
    data.modelMatrix[3][0] = -2.5f;
    SetInstanceData(&MainScene, instanceID, &data);

    instanceID = CreateInstance(&MainScene, 2);
    data.modelMatrix = m4f(0.5f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][0] = 8.0f;
    SetInstanceData(&MainScene, instanceID, &data);

    instanceID = CreateInstance(&MainScene, 2);
    data.modelMatrix = m4f(0.25f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][0] = 8.0f;
    data.modelMatrix[3][2] = 6.0f;
    SetInstanceData(&MainScene, instanceID, &data);

    instanceID = CreateInstance(&MainScene, 3);
    data.modelMatrix = m4f(7.0f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][1] = 8.0f;
    SetInstanceData(&MainScene, instanceID, &data);

    instanceID = CreateInstance(&MainScene, 3);
    data.modelMatrix = m4f(7.0f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][1] = 10.0f;
    SetInstanceData(&MainScene, instanceID, &data);

    // Procedural geometry
    CreateAnimatedPoly(&gameGraphicsData.m_animatedPolygon);
}

static void DestroyDescriptors()
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

static void WriteSwapChainBlitResources()
{
    Graphics::DescriptorSetDataHandles blitHandles = {};
    blitHandles.InitInvalid();
    blitHandles.handles[0] = gameGraphicsData.m_rtColorHandle;
    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_TEX, gameGraphicsData.m_swapChainBlitDescHandle, &blitHandles);

    Graphics::DescriptorSetDataHandles vbHandles = {};
    vbHandles.InitInvalid();
    vbHandles.handles[0] = defaultQuad.m_positionBuffer.gpuBufferHandle;
    vbHandles.handles[1] = defaultQuad.m_uvBuffer.gpuBufferHandle;
    vbHandles.handles[2] = defaultQuad.m_normalBuffer.gpuBufferHandle;
    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_VBS, defaultQuad.m_descriptor, &vbHandles);
}

static void CreateAllDescriptors()
{
    // Swap chain blit
    gameGraphicsData.m_swapChainBlitDescHandle = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_TEX);
    WriteSwapChainBlitResources();

    // Descriptor data
    Graphics::ResourceDesc desc;
    desc.resourceType = Graphics::ResourceType::eBuffer1D;
    desc.dims = v3ui(sizeof(DescriptorData_Instance) * MAX_INSTANCES_PER_SCENE, 0, 0);
    desc.bufferUsage = Graphics::BufferUsage::eUniform;
    desc.debugLabel = "Descriptor Buffer Instance Constant Data";
    gameGraphicsData.m_DescDataBufferHandle_Instance = Graphics::CreateResource(desc);
    desc.dims = v3ui(sizeof(DescriptorData_Global), 0, 0);
    desc.debugLabel = "Descriptor Buffer Global Constant Data";
    gameGraphicsData.m_DescDataBufferHandle_Global = Graphics::CreateResource(desc);

    gameGraphicsData.m_DescData_Global = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_VIEW_GLOBAL);

    Graphics::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descDataHandles[i].InitInvalid();
    descDataHandles[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Global;
    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_VIEW_GLOBAL, gameGraphicsData.m_DescData_Global, &descDataHandles[0]);

    gameGraphicsData.m_DescData_Instance = Graphics::CreateDescriptor(Graphics::DESCLAYOUT_ID_ASSET_INSTANCE);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descDataHandles[i].InitInvalid();
    descDataHandles[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Instance;
    Graphics::WriteDescriptor(Graphics::DESCLAYOUT_ID_ASSET_INSTANCE, gameGraphicsData.m_DescData_Instance, &descDataHandles[0]);
}

static void CreateGameRenderingResources(uint32 windowWidth, uint32 windowHeight)
{
    Graphics::ResourceDesc desc;
    desc.resourceType = Graphics::ResourceType::eImage2D;
    desc.arrayEles = 1;
    desc.dims = v3ui(windowWidth, windowHeight, 1);
    desc.imageFormat = Graphics::ImageFormat::RGBA8_SRGB;
    desc.debugLabel = "MainViewColor";
    gameGraphicsData.m_rtColorHandle = Graphics::CreateResource(desc);

    desc.imageFormat = Graphics::ImageFormat::Depth_32F;
    desc.debugLabel = "MainViewDepth";
    gameGraphicsData.m_rtDepthHandle = Graphics::CreateResource(desc);

    gameRenderPassList[eRenderPass_ZPrePass].Init();
    gameRenderPassList[eRenderPass_ZPrePass].numColorRTs = 0;
    gameRenderPassList[eRenderPass_ZPrePass].depthRT = gameGraphicsData.m_rtDepthHandle;
    gameRenderPassList[eRenderPass_ZPrePass].renderWidth = windowWidth;
    gameRenderPassList[eRenderPass_ZPrePass].renderHeight = windowHeight;
    gameRenderPassList[eRenderPass_ZPrePass].debugLabel = "Z Prepass";
    gameRenderPassList[eRenderPass_ZPrePass].ExecuteFn = ZPrepassRenderPass::Execute;

    gameRenderPassList[eRenderPass_MainView].Init();
    gameRenderPassList[eRenderPass_MainView].numColorRTs = 1;
    gameRenderPassList[eRenderPass_MainView].colorRTs[0] = gameGraphicsData.m_rtColorHandle;
    gameRenderPassList[eRenderPass_MainView].depthRT = gameGraphicsData.m_rtDepthHandle;
    gameRenderPassList[eRenderPass_MainView].renderWidth = windowWidth;
    gameRenderPassList[eRenderPass_MainView].renderHeight = windowHeight;
    gameRenderPassList[eRenderPass_MainView].debugLabel = "Main Forward Render View";
    gameRenderPassList[eRenderPass_MainView].ExecuteFn = ForwardRenderPass::Execute;

    gameRenderPassList[eRenderPass_DebugUI].Init();
    gameRenderPassList[eRenderPass_DebugUI].numColorRTs = 1;
    gameRenderPassList[eRenderPass_DebugUI].colorRTs[0] = gameGraphicsData.m_rtColorHandle;
    gameRenderPassList[eRenderPass_DebugUI].depthRT = Graphics::DefaultResHandle_Invalid;
    gameRenderPassList[eRenderPass_DebugUI].renderWidth = windowWidth;
    gameRenderPassList[eRenderPass_DebugUI].renderHeight = windowHeight;
    gameRenderPassList[eRenderPass_DebugUI].debugLabel = "Debug UI";
    gameRenderPassList[eRenderPass_DebugUI].ExecuteFn = DebugUIRenderPass::Execute;

    gameRenderPassList[eRenderPass_SwapChainBlit].Init();
    gameRenderPassList[eRenderPass_SwapChainBlit].numColorRTs = 1;
    gameRenderPassList[eRenderPass_SwapChainBlit].colorRTs[0] = Graphics::IMAGE_HANDLE_SWAP_CHAIN;
    gameRenderPassList[eRenderPass_SwapChainBlit].depthRT = Graphics::DefaultResHandle_Invalid;
    gameRenderPassList[eRenderPass_SwapChainBlit].renderWidth = windowWidth;
    gameRenderPassList[eRenderPass_SwapChainBlit].renderHeight = windowHeight;
    gameRenderPassList[eRenderPass_SwapChainBlit].debugLabel = "Swap Chain Blit";
    gameRenderPassList[eRenderPass_SwapChainBlit].ExecuteFn = SwapChainBlitRenderPass::Execute;
}

INPUT_CALLBACK(ToggleImGuiDisplay)
{
    DebugUI::ToggleEnable();
}

static uint32 GameInit(uint32 windowWidth, uint32 windowHeight)
{
    TIMED_SCOPED_BLOCK("Game Init");

    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;

    windowHandles = Tk::Platform::GetPlatformWindowHandles();

    // Graphics init
    Tk::Graphics::CreateContext(windowHandles, windowWidth, windowHeight);
    graphicsCommandStream = {};
    graphicsCommandStream.m_numCommands = 0;
    graphicsCommandStream.m_maxCommands = TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX;
    graphicsCommandStream.m_graphicsCommands = (Tk::Graphics::GraphicsCommand*)Tk::Core::CoreMallocAligned(graphicsCommandStream.m_maxCommands * sizeof(Tk::Graphics::GraphicsCommand), CACHE_LINE);

    if (Tk::ShaderCompiler::Init() != Tk::ShaderCompiler::ErrCode::Success)
    {
        TINKER_ASSERT(0);
        Tk::Core::Utility::LogMsg("Game", "Failed to init shader compiler!", Tk::Core::Utility::LogSeverity::eCritical);
    }
    Tk::Graphics::ShaderManager::Startup();
    Tk::Graphics::ShaderManager::LoadAllShaderResources(windowWidth, windowHeight);
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eF11, HotloadAllShaders); // Bind shader hotloading hotkey

    // Debug UI
    DebugUI::Init(&graphicsCommandStream);
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eF1, ToggleImGuiDisplay); // Toggle with hotkey - TODO: move to tilde with ctrl?

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
    g_projMat = PerspectiveProjectionMatrix((float)currentWindowWidth / currentWindowHeight);

    // Init network connection if multiplayer
    if (isMultiplayer)
    {
        int result = InitNetworkConnection();
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
        g_AssetManager.LoadAllAssets();
        g_AssetManager.InitAssetGraphicsResources(&graphicsCommandStream);
    }

    CreateDefaultGeometry(&graphicsCommandStream);

    CreateGameRenderingResources(windowWidth, windowHeight);

    InitDemo();

    CreateAllDescriptors();

    return 0;
}

extern "C"
GAME_UPDATE(GameUpdate)
{
    graphicsCommandStream.m_numCommands = 0;

    if (!isGameInitted)
    {
        uint32 initResult = GameInit(windowWidth, windowHeight);
        if (initResult != 0)
        {
            return initResult;
        }
        isGameInitted = true;
    }

    // Start frame
    bool shouldRenderFrame = Tk::Graphics::AcquireFrame();

    if (!shouldRenderFrame)
    {
        if (isWindowMinimized)
        {
            return 0; // gracefully skip this frame 
        }
        else
        {
            return 1; // acquire actually failed for some reason
            // TODO real error codes
        }
    }

    DebugUI::NewFrame();

    UpdateAxisVectors(&g_gameCamera);

    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;

    {
        //TIMED_SCOPED_BLOCK("Input manager update - kb/mouse callbacks");
        g_InputManager.UpdateAndDoCallbacks(inputStateDeltas);
    }

    // Update scene and view
    {
        Graphics::DescriptorSetDataHandles descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0].InitInvalid();
        descriptors[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Global;
        descriptors[1].InitInvalid();
        descriptors[1].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Instance;

        Update(&MainScene, descriptors);

        MainView.m_viewMatrix = CameraViewMatrix(&g_gameCamera);
        MainView.m_projMatrix = g_projMat;
        
        Update(&MainView, descriptors);
    }

    // Update Imgui menus
    DebugUI::UI_MainMenu();
    DebugUI::UI_PerformanceOverview();
    DebugUI::UI_RenderPassStats();

    // Timestamp start of frame
    {
        graphicsCommandStream.CmdTimestamp("Begin Frame", "Timestamp", true);
    }
    
    // Run the "render graph"
    {
        //TIMED_SCOPED_BLOCK("Graphics command stream recording");

        for (uint32 uiRenderPass = 0; uiRenderPass < eRenderPass_Max; ++uiRenderPass)
        {
            GameRenderPass& currRP = gameRenderPassList[uiRenderPass];
            currRP.ExecuteFn(&currRP, &graphicsCommandStream);

            graphicsCommandStream.CmdTimestamp(currRP.debugLabel);
        }
    }

    // Process recorded graphics command stream
    {
        //TIMED_SCOPED_BLOCK("Graphics command stream processing");
        Tk::Graphics::BeginFrameRecording();
        Tk::Graphics::ProcessGraphicsCommandStream(&graphicsCommandStream, false);
        Tk::Graphics::EndFrameRecording();
        Tk::Graphics::SubmitFrameToGPU();
    }

    if (isGameInitted && isMultiplayer && connectedToServer)
    {
        int result = SendMessageToServer();
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

static void DestroyWindowResizeDependentResources()
{
    Graphics::DestroyResource(gameGraphicsData.m_rtColorHandle);
    Graphics::DestroyResource(gameGraphicsData.m_rtDepthHandle);
}

extern "C"
GAME_WINDOW_RESIZE(GameWindowResize)
{
    if (newWindowWidth == 0 && newWindowHeight == 0)
    {
        Tk::Graphics::WindowMinimized();
        isWindowMinimized = true;
    }
    else
    {
        isWindowMinimized = false;
        Tk::Graphics::WindowResize();
        Tk::Graphics::ShaderManager::CreateWindowDependentResources(newWindowWidth, newWindowHeight);

        currentWindowWidth = newWindowWidth;
        currentWindowHeight = newWindowHeight;
        DestroyWindowResizeDependentResources();

        // Gameplay stuff
        g_projMat = PerspectiveProjectionMatrix((float)currentWindowWidth / currentWindowHeight);

        CreateGameRenderingResources(newWindowWidth, newWindowHeight);
        WriteSwapChainBlitResources();
    }
}

extern "C"
GAME_DESTROY(GameDestroy)
{
    if (isGameInitted)
    {
        DebugUI::Shutdown();

        DestroyWindowResizeDependentResources();
        DestroyDescriptors();

        DestroyDefaultGeometry();
        DestroyDefaultGeometryVertexBufferDescriptor(defaultQuad);
        
        DestroyAnimatedPoly(&gameGraphicsData.m_animatedPolygon);

        // Destroy assets
        g_AssetManager.DestroyAllMeshData();
        g_AssetManager.DestroyAllTextureData();

        if (isMultiplayer && connectedToServer)
        {
            EndNetworkConnection();
        }

        g_AssetManager.FreeMemory();

        // Shutdown graphics
        Tk::Graphics::ShaderManager::Shutdown();
        Tk::Graphics::DestroyContext();
        Tk::Core::CoreFreeAligned(graphicsCommandStream.m_graphicsCommands);
    }
}
