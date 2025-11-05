#include "Allocators.h"
#include "AssetFileParsing.h"
#include "AssetManager.h"
#include "BindlessSystem.h"
#include "Camera.h"
#include "DataRepository.h"
#include "DebugUI.h"
#include "GameDebugMenus.h"
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
static Platform::WindowHandles* g_windowHandles = nullptr;

#define TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX MAX_UINT16
Graphics::GraphicsCommandStream g_graphicsCommandStream;
Graphics::CommandBuffer g_FrameCommandBuffer;

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
  Core::Utility::LogMsg("Game", "Attempting to hotload shaders...\n",
                        Core::Utility::LogSeverity::eInfo);

  uint32 result = ShaderCompiler::ErrCode::NonShaderError;
#ifdef VULKAN
  result = ShaderCompiler::CompileAllShadersVK();
#else
#endif

  if (result == ShaderCompiler::ErrCode::Success)
  {
    Graphics::ShaderManager::ReloadShaders();
    Core::Utility::LogMsg("Game", "...Done.\n", Core::Utility::LogSeverity::eInfo);
  }
  else
  {
    // TODO: grab error message from shader compiler
    Core::Utility::LogMsg("Game", "Shader compilation failed.\n",
                          Core::Utility::LogSeverity::eWarning);
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

static void PushAssetTexturesBindless()
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
}

static void CreateGameRenderingResources(uint32 windowWidth, uint32 windowHeight)
{
  FrameRenderParams frameRenderParams = {
    .swapChainWidth = windowWidth,
    .swapChainHeight = windowHeight,
  };
  RenderGraph::Create(frameRenderParams);

  g_FrameCommandBuffer = Graphics::CreateCommandBuffer();
}

INPUT_CALLBACK(ToggleImGuiDisplay)
{
  ToggleEnable();
}

static uint32 GameInit(uint32 windowWidth, uint32 windowHeight)
{
  TIMED_SCOPED_BLOCK("Game Init");

  currentWindowWidth = windowWidth;
  currentWindowHeight = windowHeight;

  g_windowHandles = Platform::GetPlatformWindowHandles();

  // Graphics init
  Graphics::CreateContext(g_windowHandles);
  Graphics::CreateSwapChain(g_windowHandles, windowWidth, windowHeight);
  g_graphicsCommandStream = {
    .m_graphicsCommands = static_cast<Graphics::GraphicsCommand*>(Core::CoreMallocAligned(
      TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX * sizeof(Graphics::GraphicsCommand),
      CACHE_LINE)),
    .m_numCommands = 0,
    .m_maxCommands = TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX
  };

  if (ShaderCompiler::Init() != ShaderCompiler::ErrCode::Success)
  {
    TINKER_ASSERT(0);
    Core::Utility::LogMsg("Game", "Failed to init shader compiler!",
                          Core::Utility::LogSeverity::eCritical);
  }
  Graphics::ShaderManager::Startup();
  Graphics::ShaderManager::LoadAllShaderResources();
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

  BindlessSystem::Create();
  CreateGameRenderingResources(windowWidth, windowHeight);

  InitDemo();

  return 0;
}

static void RecordFrameRenderCommands(const FrameRenderParams& frameRenderParams)
{
  g_graphicsCommandStream.CmdCommandBufferBegin(g_FrameCommandBuffer,
                                                "Begin game frame cmd buffer");
  g_graphicsCommandStream.CmdTimestampReadback("TimestampReadback");
  g_graphicsCommandStream.CmdTimestamp("Begin Frame", "Timestamp");

  RenderGraph::Run(&g_graphicsCommandStream, frameRenderParams, g_windowHandles);

  g_graphicsCommandStream.CmdCommandBufferEnd(g_FrameCommandBuffer);
}

static void UpdateGPUData(const FrameRenderParams& frameRenderParams)
{
  // Update bindless resource descriptors
  // TODO: this will eventually be automatically managed by
  // some material system (maybe even tracks what's currently
  // in the scene)
  PushAssetTexturesBindless();

  RenderGraph::Prepare(frameRenderParams);

  DataRepo::FlushShaderConstants();

  // Update scene
  // Order matters, this pushes instance constants
  // which must be pushed after globals above.
  {
    PrepareToRender(&MainScene);
  }

  BindlessSystem::Flush();
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
  bool shouldRenderFrame = Graphics::AcquireFrame(g_windowHandles);

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

  // Update view
  {
    MainView.m_viewMatrix = CameraViewMatrix(&g_gameCamera);
    MainView.m_projMatrix = g_projMat;
    MainView.Update();
  }

  {
    // Write misc data to the data repository
    alignas(16) v4f camPosition = v4f(g_gameCamera.m_eye, 1.0f);
    DataRepo::g_theDataRepository.ShaderConstants_Globals.CamPosition = camPosition;
  }

  {
    Update(&MainScene);
  }

  FrameRenderParams frameRenderParams = {
    .swapChainWidth = currentWindowWidth,
    .swapChainHeight = currentWindowHeight,
  };
  UpdateGPUData(frameRenderParams);

  // Update Imgui menus
  UpdateAllDebugMenus();

  RecordFrameRenderCommands(frameRenderParams);

  // Process recorded graphics command stream
  {
    //TIMED_SCOPED_BLOCK("Graphics command stream processing");

    Graphics::ProcessGraphicsCommandStream(&g_graphicsCommandStream);
    Graphics::SubmitFrameToGPU(g_windowHandles, g_FrameCommandBuffer);
    Graphics::PresentToSwapChain(g_windowHandles);
    g_graphicsCommandStream.Clear();

    // Debug UI - extra submissions
    DebugUI::RenderAndSubmitMultiViewports(&g_graphicsCommandStream);

    Graphics::EndFrame();
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
    Graphics::WindowMinimized(windowHandles);
    isWindowMinimized = true;
  }
  else
  {
    isWindowMinimized = false;
    Graphics::WindowResize(windowHandles, newWindowWidth, newWindowHeight);

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
    Graphics::ShaderManager::Shutdown();
    Graphics::DestroySwapChain(g_windowHandles);
    Graphics::DestroyContext();
    Core::CoreFreeAligned(g_graphicsCommandStream.m_graphicsCommands);
  }
}
