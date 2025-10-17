#include "Allocators.h"
#include "AssetFileParsing.h"
#include "AssetManager.h"
#include "BindlessSystem.h"
#include "Camera.h"
#include "DebugUI.h"
#include "Generated/ShaderDescriptors_Reflection.h"
#include "Graphics/Common/GPUTimestamps.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "Graphics/Common/ShaderManager.h"
#include "GraphicsTypes.h"
#include "Hashing.h"
#include "InputManager.h"
#include "Math/VectorTypes.h"
#include "Platform/PlatformGameAPI.h"
#include "Raytracing.h"
#include "RenderGraph.h"
#include "Scene.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "Utility/ScopedTimer.h"
#include "View.h"
#include <string.h>

using namespace Tk;
using namespace Platform;

static bool isGameInitted = false;
static const bool isMultiplayer = false;
static bool connectedToServer = false;
static uint32 currentWindowWidth = 0;
static uint32 currentWindowHeight = 0;
static bool isWindowMinimized;
static Tk::Platform::WindowHandles* g_windowHandles = nullptr;

#define TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX MAX_UINT16
Tk::Graphics::GraphicsCommandStream g_graphicsCommandStream;
Tk::Graphics::CommandBuffer g_FrameCommandBuffer;

// For now, this owns all RTs
GameGraphicsData gameGraphicsData = {};

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
  Tk::Core::Utility::LogMsg("Game", "Attempting to hotload shaders...\n",
                            Tk::Core::Utility::LogSeverity::eInfo);

  uint32 result = Tk::ShaderCompiler::ErrCode::NonShaderError;
#ifdef VULKAN
  result = Tk::ShaderCompiler::CompileAllShadersVK();
#else
#endif

  if (result == Tk::ShaderCompiler::ErrCode::Success)
  {
    Tk::Graphics::ShaderManager::ReloadShaders();
    Tk::Core::Utility::LogMsg("Game", "...Done.\n",
                              Tk::Core::Utility::LogSeverity::eInfo);
  }
  else
  {
    // TODO: grab error message from shader compiler
    Tk::Core::Utility::LogMsg("Game", "Shader compilation failed.\n",
                              Tk::Core::Utility::LogSeverity::eWarning);
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
  ShaderDescriptors::InstanceData_Basic data;
  data.ModelMatrix = m4f(1.0f);

  MainView.Init();
  uint32 instanceID;
  instanceID = CreateInstance(&MainScene, 0);
  data.ModelMatrix[3][0] = -3.0f;
  SetInstanceData(&MainScene, instanceID, &data);

  instanceID = CreateInstance(&MainScene, 1);
  data.ModelMatrix[3][0] = -2.0f;
  SetInstanceData(&MainScene, instanceID, &data);

  instanceID = CreateInstance(&MainScene, 2);
  data.ModelMatrix = m4f(0.5f);
  data.ModelMatrix[3][3] = 1.0f;
  data.ModelMatrix[3][0] = 4.0f;
  SetInstanceData(&MainScene, instanceID, &data);

  instanceID = CreateInstance(&MainScene, 2);
  data.ModelMatrix = m4f(0.25f);
  data.ModelMatrix[3][3] = 1.0f;
  data.ModelMatrix[3][0] = 5.0f;
  data.ModelMatrix[3][2] = 2.0f;
  SetInstanceData(&MainScene, instanceID, &data);

  instanceID = CreateInstance(&MainScene, 3);
  data.ModelMatrix = m4f(0.5f);
  data.ModelMatrix[3][3] = 1.0f;
  data.ModelMatrix[3][0] = -1.0f;
  data.ModelMatrix[3][1] = 2.0f;
  SetInstanceData(&MainScene, instanceID, &data);

  instanceID = CreateInstance(&MainScene, 3);
  data.ModelMatrix = m4f(1.0f);
  data.ModelMatrix[3][3] = 1.0f;
  data.ModelMatrix[3][0] = 1.0f;
  data.ModelMatrix[3][1] = 4.0f;
  SetInstanceData(&MainScene, instanceID, &data);

  // Procedural geometry
  CreateAnimatedPoly(&gameGraphicsData.m_animatedPolygon);
}

static void DestroyDescriptors()
{
  BindlessSystem::Destroy();
  Graphics::DestroyAllDescriptors(); // destroys descriptor pool
}

static void RegisterActiveTextures()
{
  uint32 index = BindlessSystem::BindlessIndexMax;
  index = BindlessSystem::BindResourceForFrame(
    g_AssetManager.GetTextureGraphicsDataByID(0),
    BindlessSystem::BindlessArrayID::eTexturesRGBA8Sampled);
  index = BindlessSystem::BindResourceForFrame(
    g_AssetManager.GetTextureGraphicsDataByID(1),
    BindlessSystem::BindlessArrayID::eTexturesRGBA8Sampled);
  // TODO: eventually these indices will be hooked up to a material system so that at draw
  // time we can pass these indices as a constant to the gpu for bindless descriptor
  // indexing

  // WIP: Push some render targets into the bindless array for compute copy test pass
  // TODO: move this struct building to elsewhere
  alignas(16) ShaderDescriptors::Material_ComputeCopyImage2D copyConstants = {};
  copyConstants.dims = v2ui(currentWindowWidth, currentWindowHeight);
  copyConstants.srcIndexBindless = BindlessSystem::BindResourceForFrame(
    gameGraphicsData.m_rtColorToneMappedHandle,
    BindlessSystem::BindlessArrayID::eTexturesRGBA8RW);
  copyConstants.dstIndexBindless = BindlessSystem::BindResourceForFrame(
    gameGraphicsData.m_computeColorHandle,
    BindlessSystem::BindlessArrayID::eTexturesRGBA8RW);
  uint32 materialDataByteOffset = BindlessSystem::PushStructIntoConstantBuffer(
    &copyConstants, sizeof(copyConstants),
    alignof(ShaderDescriptors::Material_ComputeCopyImage2D));
}

static void CreateAllDescriptors()
{
  BindlessSystem::Create();
}

static void CreateGameRenderingResources(uint32 windowWidth, uint32 windowHeight)
{
  FrameRenderParams frameRenderParams = {
    .swapChainWidth = windowWidth,
    .swapChainHeight = windowHeight,
  };
  RenderGraph::Create(frameRenderParams);

  g_FrameCommandBuffer = Tk::Graphics::CreateCommandBuffer();
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

  g_windowHandles = Tk::Platform::GetPlatformWindowHandles();

  // Graphics init
  Tk::Graphics::CreateContext(g_windowHandles);
  Tk::Graphics::CreateSwapChain(g_windowHandles, windowWidth, windowHeight);
  g_graphicsCommandStream = {};
  g_graphicsCommandStream.m_numCommands = 0;
  g_graphicsCommandStream.m_maxCommands = TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX;
  g_graphicsCommandStream.m_graphicsCommands =
    (Tk::Graphics::GraphicsCommand*)Tk::Core::CoreMallocAligned(
      g_graphicsCommandStream.m_maxCommands * sizeof(Tk::Graphics::GraphicsCommand),
      CACHE_LINE);

  if (Tk::ShaderCompiler::Init() != Tk::ShaderCompiler::ErrCode::Success)
  {
    TINKER_ASSERT(0);
    Tk::Core::Utility::LogMsg("Game", "Failed to init shader compiler!",
                              Tk::Core::Utility::LogSeverity::eCritical);
  }
  Tk::Graphics::ShaderManager::Startup();
  Tk::Graphics::ShaderManager::LoadAllShaderResources();
  g_InputManager.BindKeycodeCallback_KeyDown(
    Platform::Keycode::eF11, HotloadAllShaders); // Bind shader hotloading hotkey

  // Debug UI
  DebugUI::Init(&g_graphicsCommandStream);
  g_InputManager.BindKeycodeCallback_KeyDown(
    Platform::Keycode::eF1,
    ToggleImGuiDisplay); // Toggle with hotkey - TODO: move to tilde with ctrl?

  // Camera controls
  g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eW,
                                             GameCameraPanForwardCallback);
  g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eA,
                                             GameCameraPanLeftCallback);
  g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eS,
                                             GameCameraPanBackwardCallback);
  g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eD,
                                             GameCameraPanRightCallback);
  g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eW,
                                                   GameCameraPanForwardCallback);
  g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eA,
                                                   GameCameraPanLeftCallback);
  g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eS,
                                                   GameCameraPanBackwardCallback);
  g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eD,
                                                   GameCameraPanRightCallback);
  g_InputManager.BindMousecodeCallback(Platform::Mousecode::eMouseMoveHorizontal,
                                       GameCameraRotateHorizontalCallback);
  g_InputManager.BindMousecodeCallback(Platform::Mousecode::eMouseMoveVertical,
                                       GameCameraRotateVerticalCallback);

  // Hotkeys
  g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eF9,
                                             RaytraceTestCallback);

  g_gameCamera.m_ref = v3f(0.0f, 0.0f, 0.0f);
  g_gameCamera.m_eye = v3f(7.0f, -7.0f, 7.0f);
  g_projMat =
    PerspectiveProjectionMatrix((float)currentWindowWidth / currentWindowHeight);

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
    g_AssetManager.InitAssetGraphicsResources(&g_graphicsCommandStream);
  }

  CreateDefaultGeometry(&g_graphicsCommandStream);
  Graphics::CreateAllDefaultResources(&g_graphicsCommandStream);

  CreateGameRenderingResources(windowWidth, windowHeight);

  InitDemo();

  return 0;
}

extern "C" GAME_UPDATE(GameUpdate)
{
  g_graphicsCommandStream.Clear();

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
  bool shouldRenderFrame = Tk::Graphics::AcquireFrame(g_windowHandles);

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

  BindlessSystem::ResetFrame();

  // Update Imgui menus
  DebugUI::UI_MainMenu();
  DebugUI::UI_PerformanceOverview();
  DebugUI::UI_RenderPassStats();
  {
    // TODO: put this in View::Update() and write to the data repository from there
    alignas(16) m4f viewProj = g_projMat * CameraViewMatrix(&g_gameCamera);
    v4f camPosition = v4f(g_gameCamera.m_eye, 1.0f);
    uint32 firstGlobalDataByteOffset = BindlessSystem::PushStructIntoConstantBuffer(
      &viewProj, sizeof(viewProj), alignof(m4f));
    BindlessSystem::PushStructIntoConstantBuffer(&viewProj, sizeof(camPosition),
                                                 alignof(v4f));
    TINKER_ASSERT(firstGlobalDataByteOffset == 0);
    (void)firstGlobalDataByteOffset;
  }

  // Update bindless resource descriptors
  RegisterActiveTextures(); // TODO: this will eventually be automatically managed by some
                            // material system (maybe even tracks what's currently in the
                            // scene)

  // Update scene and view
  {
    Update(&MainScene);

    MainView.m_viewMatrix = CameraViewMatrix(&g_gameCamera);
    MainView.m_projMatrix = g_projMat;
    MainView.Update();
  }

  BindlessSystem::Flush();

  {
    g_graphicsCommandStream.CmdCommandBufferBegin(g_FrameCommandBuffer,
                                                  "Begin game frame cmd buffer");
  }

  // Timestamp start of frame
  {
    g_graphicsCommandStream.CmdTimestamp("Begin Frame", "Timestamp", true);
  }

  FrameRenderParams frameRenderParams = {
    .swapChainWidth = windowWidth,
    .swapChainHeight = windowHeight,
  };
  RenderGraph::Run(&g_graphicsCommandStream, frameRenderParams, g_windowHandles);

  g_graphicsCommandStream.CmdCommandBufferEnd(g_FrameCommandBuffer);

  // Process recorded graphics command stream
  {
    //TIMED_SCOPED_BLOCK("Graphics command stream processing");

    Tk::Graphics::ProcessGraphicsCommandStream(&g_graphicsCommandStream);
    Tk::Graphics::SubmitFrameToGPU(g_windowHandles, g_FrameCommandBuffer);
    Tk::Graphics::PresentToSwapChain(g_windowHandles);
    g_graphicsCommandStream.Clear();

    // Debug UI - extra submissions
    DebugUI::RenderAndSubmitMultiViewports(&g_graphicsCommandStream);

    Tk::Graphics::EndFrame();
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

extern "C" GAME_WINDOW_RESIZE(GameWindowResize)
{
  if (newWindowWidth == 0 && newWindowHeight == 0)
  {
    Tk::Graphics::WindowMinimized(windowHandles);
    isWindowMinimized = true;
  }
  else
  {
    isWindowMinimized = false;
    Tk::Graphics::WindowResize(windowHandles, newWindowWidth, newWindowHeight);

    currentWindowWidth = newWindowWidth;
    currentWindowHeight = newWindowHeight;
    RenderGraph::Destroy();

    // Gameplay stuff
    g_projMat =
      PerspectiveProjectionMatrix((float)currentWindowWidth / currentWindowHeight);

    CreateGameRenderingResources(newWindowWidth, newWindowHeight);
  }
}

extern "C" GAME_DESTROY(GameDestroy)
{
  if (isGameInitted)
  {
    DebugUI::Shutdown();

    RenderGraph::Destroy();
    DestroyDescriptors();

    DestroyDefaultGeometry();
    Graphics::DestroyDefaultResources();

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
    Tk::Graphics::DestroySwapChain(g_windowHandles);
    Tk::Graphics::DestroyContext();
    Tk::Core::CoreFreeAligned(g_graphicsCommandStream.m_graphicsCommands);
  }
}
