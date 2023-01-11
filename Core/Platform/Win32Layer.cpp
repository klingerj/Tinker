#include "CoreDefines.h"
#include "PlatformCommon.h"
#include "PlatformGameAPI.h"
#include "Win32WorkerThreadPool.h"
#include "Win32Client.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "Graphics/Common/ShaderManager.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "Utility/Logging.h"
#include "Utility/ScopedTimer.h"
#include "CoreDebugUI.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Windowsx.h>

#include "imgui.h"
#include "backends/imgui_impl_win32.h"

// TODO: make these to be compile defines
#define TINKER_PLATFORM_ENABLE_MULTITHREAD
#ifndef TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX
#define TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX MAX_UINT16
#endif
#ifndef TINKER_PLATFORM_HOTLOAD_FILENAME
#define TINKER_PLATFORM_HOTLOAD_FILENAME "TinkerGame_hotload.dll"
#endif

static GAME_UPDATE(GameUpdateStub) { return 0; }
static GAME_DESTROY(GameDestroyStub) {}
static GAME_WINDOW_RESIZE(GameWindowResizeStub) {}
typedef struct win32_game_code
{
    HMODULE GameDll = 0;
    FILETIME lastWriteTime = {};
    Tk::Platform::game_update* GameUpdate = GameUpdateStub;
    Tk::Platform::game_destroy* GameDestroy = GameDestroyStub;
    Tk::Platform::game_window_resize* GameWindowResize = GameWindowResizeStub;
} Win32GameCode;

Win32GameCode g_GameCode;
const bool enableDllHotloading = true;

volatile bool runGame = true;

HWND g_windowHandle = NULL;
Tk::Core::Graphics::GraphicsCommandStream g_graphicsCommandStream;
bool g_windowResized = false;

Tk::Platform::InputStateDeltas g_inputStateDeltas;

HCURSOR g_cursor = NULL;
bool g_cursorLocked = false;

#ifdef _SCRIPTS_DIR
#define SCRIPTS_PATH STRINGIFY(_SCRIPTS_DIR)
#else
#endif

#ifdef _GAME_DLL_PATH
#define GAME_DLL_PATH STRINGIFY(_GAME_DLL_PATH)
#else
#endif

#ifdef _GAME_DLL_HOTLOADCOPY_PATH
#define GAME_DLL_HOTLOADCOPY_PATH STRINGIFY(_GAME_DLL_HOTLOADCOPY_PATH)
#else
#endif

typedef struct global_app_params
{
    uint32 m_windowWidth;
    uint32 m_windowHeight;
} GlobalAppParams;
GlobalAppParams g_GlobalAppParams;

SYSTEM_INFO g_SystemInfo;
Tk::Platform::PlatformWindowHandles g_platformWindowHandles;

static bool ReloadGameCode(Win32GameCode* GameCode)
{
    if (!enableDllHotloading)
        return false;

    const char* str = GAME_DLL_PATH;
    HANDLE gameDllFileHandle = CreateFile(GAME_DLL_PATH, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (gameDllFileHandle == INVALID_HANDLE_VALUE)
    {
        DWORD errorCode = GetLastError();
        if (errorCode == 32)
        {
            // game dll currently being used by another process - not a big deal, probably being written to during build. Don't log an error
        }
        else
        {
            // some other error, log a failure
            Tk::Core::Utility::LogMsg("Platform", "Failed to get handle to game dll to reload!", Tk::Core::Utility::LogSeverity::eCritical);
        }

        // Regardless of error code, don't continue with the hotload attempt
        return false;
    }

    // Check the game dll's last write time, and reload it if it has been updated
    FILETIME gameDllLastWriteTime;
    GetFileTime(gameDllFileHandle, 0, 0, &gameDllLastWriteTime);

    CloseHandle(gameDllFileHandle);
    if (CompareFileTime(&gameDllLastWriteTime, &GameCode->lastWriteTime))
    {
        Tk::Core::Utility::LogMsg("Platform", "Loading game dll!", Tk::Core::Utility::LogSeverity::eInfo);
        Tk::Core::Utility::LogMsg("Platform", GAME_DLL_PATH, Tk::Core::Utility::LogSeverity::eInfo);

        // Unload old code
        if (GameCode->GameDll)
        {
            GameCode->GameDestroy();
            FreeLibrary(GameCode->GameDll);
            GameCode->GameUpdate = GameUpdateStub;
            GameCode->GameDestroy = GameDestroyStub;
            GameCode->GameWindowResize = GameWindowResizeStub;
        }

        CopyFile(GAME_DLL_PATH, GAME_DLL_HOTLOADCOPY_PATH, FALSE);
        GameCode->GameDll = LoadLibrary(GAME_DLL_HOTLOADCOPY_PATH);
        if (GameCode->GameDll)
        {
            GameCode->GameUpdate = (Tk::Platform::game_update*)GetProcAddress(GameCode->GameDll, "GameUpdate");
            GameCode->GameDestroy = (Tk::Platform::game_destroy*)GetProcAddress(GameCode->GameDll, "GameDestroy");
            GameCode->GameWindowResize = (Tk::Platform::game_window_resize*)GetProcAddress(GameCode->GameDll, "GameWindowResize");
            GameCode->lastWriteTime = gameDllLastWriteTime;
            return true; // reload successfully
        }
    }

    return false; // didn't reload
}

namespace Tk
{
namespace Platform
{

ENQUEUE_WORKER_THREAD_JOB(EnqueueWorkerThreadJob)
{
#ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
    ThreadPool::EnqueueSingleJob(Job);
#else
    (*Job)();
    Job->m_done = 1;
#endif
}

ENQUEUE_WORKER_THREAD_JOB_LIST(EnqueueWorkerThreadJobList_Unassisted)
{
#ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
    ThreadPool::EnqueueJobList(JobList);
#else
    for (uint32 uiJob = 0; uiJob < JobList->m_numJobs; ++uiJob)
    {
        (*(JobList->m_jobs[uiJob]))();
        JobList->m_jobs[uiJob]->m_done = 1;
    }
#endif
}

ENQUEUE_WORKER_THREAD_JOB_LIST(EnqueueWorkerThreadJobList_Assisted)
{
#ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
    uint32 NumJobs = JobList->m_numJobs;
    uint32 NumThreads = ThreadPool::NumWorkerThreads() + 1;
    uint32 NumJobsPerThread = NumJobs / NumThreads;
    uint32 NumJobsLeftover = NumJobs % NumThreads;

    uint32 NumMainThreadJobs = NumJobsPerThread; // main thread never does any leftover jobs for now
    ThreadPool::EnqueueJobSubList(JobList, NumJobs - NumMainThreadJobs);

    // Main thread work
    for (uint32 uiJob = NumJobs - NumMainThreadJobs; uiJob < JobList->m_numJobs; ++uiJob)
    {
        (*(JobList->m_jobs[uiJob]))();
        JobList->m_jobs[uiJob]->m_done = 1;
    }

#else
    for (uint32 uiJob = 0; uiJob < JobList->m_numJobs; ++uiJob)
    {
        (*(JobList->m_jobs[uiJob]))();
        JobList->m_jobs[uiJob]->m_done = 1;
    }
#endif
}

INIT_NETWORK_CONNECTION(InitNetworkConnection)
{
    if (Network::InitClient() != 0)
    {
        Tk::Core::Utility::LogMsg("Platform", "Failed to init network client!", Tk::Core::Utility::LogSeverity::eCritical);
        return 1;
    }
    else
    {
        Tk::Core::Utility::LogMsg("Platform", "Successfully initialized network client.", Tk::Core::Utility::LogSeverity::eInfo);
        return 0;
    }
}

END_NETWORK_CONNECTION(EndNetworkConnection)
{
    if (Network::DisconnectFromServer() != 0)
    {
        Tk::Core::Utility::LogMsg("Platform", "Failed to cleanup network client!", Tk::Core::Utility::LogSeverity::eCritical);
        return 1;
    }
    else
    {
        Tk::Core::Utility::LogMsg("Platform", "Successfully cleaned up network client.", Tk::Core::Utility::LogSeverity::eInfo);
        return 0;
    }
}

SEND_MESSAGE_TO_SERVER(SendMessageToServer)
{
    if (Network::SendMessageToServer() != 0)
    {
        Tk::Core::Utility::LogMsg("Platform", "Failed to send message to server!", Tk::Core::Utility::LogSeverity::eCritical);
        return 1;
    }
    else
    {
        return 0;
    }
}

}
}

static void LockCursor(HWND windowHandle)
{
    POINT screenCenter = { (LONG)g_GlobalAppParams.m_windowWidth / 2, (LONG)g_GlobalAppParams.m_windowHeight / 2 };
    ClientToScreen(windowHandle, &screenCenter);
    SetCursorPos(screenCenter.x, screenCenter.y);
}

static void ToggleCursorLocked()
{
    g_cursorLocked = !g_cursorLocked;
    if (g_cursorLocked)
    {
        LockCursor(g_windowHandle);
    }
    ShowCursor(!g_cursorLocked);
}

static void HandleKeypressInput(uint32 win32Keycode, uint64 win32Flags)
{
    using namespace Tk;
    using namespace Platform;
    
    uint8 wasDown = (win32Flags & (1 << 30)) == 1;
    uint8 isDown = (win32Flags & (1 << 31)) == 0;

    uint8 gameKeyCode = Keycode::eMax;

    switch (win32Keycode)
    {
        case 'W':
        {
            gameKeyCode = Keycode::eW;
            break;
        }

        case 'A':
        {
            gameKeyCode = Keycode::eA;
            break;
        }

        case 'S':
        {
            gameKeyCode = Keycode::eS;
            break;
        }

        case 'D':
        {
            gameKeyCode = Keycode::eD;
            break;
        }

        case VK_F9:
        {
            gameKeyCode = Keycode::eF9;
            break;
        }

        case VK_F10:
        {
            if (isDown)
            {
                Tk::Core::Utility::LogMsg("Platform", "Attempting to hotload shaders...\n", Tk::Core::Utility::LogSeverity::eInfo);

                uint32 result = Tk::ShaderCompiler::ErrCode::NonShaderError;
                #ifdef VULKAN
                result = Tk::ShaderCompiler::CompileAllShadersVK();
                #else
                #endif

                if (result == Tk::ShaderCompiler::ErrCode::Success)
                {
                    Tk::Core::Graphics::ShaderManager::ReloadShaders(g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
                }
                else
                {
                    // TODO: grab error message from shader compiler
                    Tk::Core::Utility::LogMsg("Platform", "Shader compilation failed.\n", Tk::Core::Utility::LogSeverity::eWarning);
                }

                Tk::Core::Utility::LogMsg("Platform", "...Done.\n", Tk::Core::Utility::LogSeverity::eInfo);
            }

            gameKeyCode = Keycode::eF10;
            break;
        }

        case VK_F11:
        {
            gameKeyCode = Keycode::eF11;
            break;
        }

        case VK_ESCAPE:
        {
            if (isDown)
            {
                ToggleCursorLocked();
            }
            break;
        }

        default:
        {
            return;
        }
    }
 
    if (gameKeyCode < Keycode::eMax)
    {
        if (!isDown || (isDown && !wasDown))
        {
            ++g_inputStateDeltas.keyCodes[gameKeyCode].numStateChanges;
        }
        g_inputStateDeltas.keyCodes[gameKeyCode].isDown = isDown;
    }
}

static void HandleMouseInput(uint32 mouseCode, int displacement)
{
    using namespace Tk;
    using namespace Platform;
    
    POINT screenCenter = { (LONG)g_GlobalAppParams.m_windowWidth / 2, (LONG)g_GlobalAppParams.m_windowHeight / 2 };
    int pxDisp = 0;

    switch (mouseCode)
    {
        case Mousecode::eMouseMoveVertical:
        {
            pxDisp = displacement - screenCenter.y;
            break;
        }

        case Mousecode::eMouseMoveHorizontal:
        {
            pxDisp = displacement - screenCenter.x;
            break;
        }

        default:
        {
            break;
        }
    }

    g_inputStateDeltas.mouseCodes[mouseCode].displacement = pxDisp;
}

LRESULT CALLBACK WindowProc(HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    using namespace Tk;
    using namespace Platform;
    
    LRESULT result = 0;

    switch (uMsg)
    {
        case WM_CREATE:
        {
            break;
        }
        case WM_SIZE:
        {
            if (wParam == 1) // window minimized - can't create swap chain with size 0
            {
                Tk::Core::Graphics::WindowMinimized();
                g_GlobalAppParams.m_windowWidth = 0;
                g_GlobalAppParams.m_windowHeight = 0;
            }
            else
            {
                // Normal window resize / maximize
                uint32 newWindowWidth = LOWORD(lParam);
                uint32 newWindowHeight = HIWORD(lParam);
                if (newWindowWidth != g_GlobalAppParams.m_windowWidth ||
                    newWindowHeight != g_GlobalAppParams.m_windowHeight)
                {
                    g_GlobalAppParams.m_windowWidth = newWindowWidth;
                    g_GlobalAppParams.m_windowHeight = newWindowHeight;
                    g_windowResized = true; // swap chain will be recreated later when this flag is checked
                }
            }

            break;
        }
        case WM_DESTROY:
        {
            break;
        }
        case WM_CLOSE:
        {
            PostQuitMessage(0);
            runGame = false;
            break;
        }
        case WM_ACTIVATEAPP:
        {
            DWORD procPrior = NORMAL_PRIORITY_CLASS;
            if (wParam) procPrior = ABOVE_NORMAL_PRIORITY_CLASS;
            if (!SetPriorityClass(GetCurrentProcess(), procPrior))
            {
                Tk::Core::Utility::LogMsg("Platform", "Failed to change process priority when changing window focus!", Tk::Core::Utility::LogSeverity::eCritical);
            }
            else
            {
                Tk::Core::Utility::LogMsg("Platform", "Changing process priority!", Tk::Core::Utility::LogSeverity::eInfo);
                if (wParam)
                {
                    Tk::Core::Utility::LogMsg("Platform", "ABOVE_NORMAL", Tk::Core::Utility::LogSeverity::eInfo);
                }
                else
                {
                    Tk::Core::Utility::LogMsg("Platform", "NORMAL", Tk::Core::Utility::LogSeverity::eInfo);
                }
            }

            if (g_cursorLocked)
            {
                LockCursor(g_windowHandle);
                SetCursorPos((int)g_GlobalAppParams.m_windowWidth / 2, (int)g_GlobalAppParams.m_windowHeight / 2);
            }

            break;
        }

        case WM_SETCURSOR:
        {
            SetCursor(g_cursor);
            break;
        }
        
        case WM_MOUSEMOVE:
        {
            if (g_cursorLocked)
            {
                int xPos = GET_X_LPARAM(lParam);
                int yPos = GET_Y_LPARAM(lParam);
                HandleMouseInput(Mousecode::eMouseMoveVertical, yPos);
                HandleMouseInput(Mousecode::eMouseMoveHorizontal, xPos);
                LockCursor(g_windowHandle);
            }
            break;
        }

        default:
        {
            result = DefWindowProc(hwnd, uMsg, wParam, lParam);
            break;
        }
    }

    return result;
}

static void ProcessWindowMessages()
{
    g_inputStateDeltas = {};

    MSG msg = {};
    while (1)
    {
        BOOL Result = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);

        if (Result)
        {
            switch (msg.message)
            {
                case WM_SYSKEYUP:
                case WM_SYSKEYDOWN:
                case WM_KEYUP:
                case WM_KEYDOWN:
                {
                    HandleKeypressInput((uint32)msg.wParam, (uint64)msg.lParam);
                    break;
                }

                default:
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                    break;
                }
            }
        }
        else
        {
            // Done
            break;
        }
    }
}

int WINAPI
wWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    PWSTR pCmdLine,
    int nCmdShow)
{
    using namespace Tk;
    using namespace Platform;
    
    {
        TIMED_SCOPED_BLOCK("Platform init");

        // TODO: load from settings file
        g_GlobalAppParams = {};
        g_GlobalAppParams.m_windowWidth = 800;
        g_GlobalAppParams.m_windowHeight = 600;

        // Get system info
        g_SystemInfo = {};
        GetSystemInfo(&g_SystemInfo);

        g_cursor = LoadCursor(NULL, IDC_ARROW);

        // Setup window
        WNDCLASS windowClass = {};
        windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = hInstance;
        //windowClass.hIcon = ;
        windowClass.hCursor = g_cursor;
        windowClass.lpszClassName = "Tinker Platform Window";
        if (!RegisterClass(&windowClass))
        {
            Tk::Core::Utility::LogMsg("Platform", "Failed to register window class!", Tk::Core::Utility::LogSeverity::eCritical);
            return 1;
        }

        RECT windowDims = { 0, 0, (LONG)g_GlobalAppParams.m_windowWidth, (LONG)g_GlobalAppParams.m_windowHeight };
        AdjustWindowRect(&windowDims, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

        g_windowHandle =
            CreateWindowEx(0,
                windowClass.lpszClassName,
                "Tinker",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                windowDims.right - windowDims.left,
                windowDims.bottom - windowDims.top,
                0,
                0,
                hInstance,
                0);

        if (!g_windowHandle)
        {
            Tk::Core::Utility::LogMsg("Platform", "Failed to create window!", Tk::Core::Utility::LogSeverity::eCritical);
            return 1;
        }

        g_platformWindowHandles = {};
        g_platformWindowHandles.instance = hInstance;
        g_platformWindowHandles.windowHandle = g_windowHandle;
        Tk::Core::Graphics::CreateContext(&g_platformWindowHandles, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);

        g_graphicsCommandStream = {};
        g_graphicsCommandStream.m_numCommands = 0;
        g_graphicsCommandStream.m_maxCommands = TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX;
        g_graphicsCommandStream.m_graphicsCommands = (Tk::Core::Graphics::GraphicsCommand*)Tk::Core::CoreMallocAligned(g_graphicsCommandStream.m_maxCommands * sizeof(Tk::Core::Graphics::GraphicsCommand), CACHE_LINE);

        #ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
        ThreadPool::Startup(g_SystemInfo.dwNumberOfProcessors / 2);
        #endif

        if (Tk::ShaderCompiler::Init() != Tk::ShaderCompiler::ErrCode::Success)
        {
            TINKER_ASSERT(0);
            Tk::Core::Utility::LogMsg("Platform", "Failed to init shader compiler!", Tk::Core::Utility::LogSeverity::eCritical);
        }
        Tk::Core::Graphics::ShaderManager::Startup();
        Tk::Core::Graphics::ShaderManager::LoadAllShaderResources(g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);

        g_GameCode = {};
        bool reloaded = ReloadGameCode(&g_GameCode);

        // Input handling
        g_inputStateDeltas = {};
        g_cursorLocked = true;
        ShowCursor(FALSE);

        // ImGui startup
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(g_windowHandle);
        Tk::Core::DebugUI::Init();
    }

    // Main loop
    while (runGame)
    {
        {
            //TIMED_SCOPED_BLOCK("-----> Total Frame");

            {
                //TIMED_SCOPED_BLOCK("Process window messages");
                ProcessWindowMessages();
            }

            {
                //TIMED_SCOPED_BLOCK("Window resize check");
                if (g_windowResized)
                {
                    Tk::Core::Graphics::WindowResize();
                    Tk::Core::Graphics::ShaderManager::CreateWindowDependentResources(g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
                    g_GameCode.GameWindowResize(g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
                    g_windowResized = false;
                }
            }

            bool shouldRenderFrame = false;
            {
                //TIMED_SCOPED_BLOCK("Acquire Frame");
                shouldRenderFrame = Tk::Core::Graphics::AcquireFrame();
            }

            if (shouldRenderFrame)
            {
                ImGui_ImplWin32_NewFrame();
                ImGui::NewFrame();
                Tk::Core::DebugUI::UI_RenderPassStats();

                {
                    //TIMED_SCOPED_BLOCK("Game Update");

                    int error = g_GameCode.GameUpdate(&g_graphicsCommandStream, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight, &g_inputStateDeltas);
                    if (error != 0)
                    {
                        Tk::Core::Utility::LogMsg("Platform", "Error occurred in game code! Shutting down application.", Tk::Core::Utility::LogSeverity::eCritical);
                        runGame = false;
                        break;
                    }
                }

                ImGui::Render();

                // Process command stream
                {
                    //TIMED_SCOPED_BLOCK("Graphics command stream processing");
                    Tk::Core::Graphics::BeginFrameRecording();
                    Tk::Core::DebugUI::Render(&g_graphicsCommandStream);
                    Tk::Core::Graphics::ProcessGraphicsCommandStream(&g_graphicsCommandStream, false);
                    Tk::Core::Graphics::EndFrameRecording();
                    Tk::Core::Graphics::SubmitFrameToGPU();
                }
            }
        }

        if (ReloadGameCode(&g_GameCode))
        {
            // Reset application resources
            #ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
            ThreadPool::Shutdown();
            ThreadPool::Startup(g_SystemInfo.dwNumberOfProcessors / 2);
            #endif

            Tk::Core::Graphics::RecreateContext(&g_platformWindowHandles, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
            Tk::Core::Graphics::ShaderManager::Shutdown();
            Tk::Core::Graphics::ShaderManager::Startup();
            Tk::Core::Graphics::ShaderManager::LoadAllShaderResources(g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
        }
    }

    g_GameCode.GameDestroy();

    #ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
    ThreadPool::Shutdown();
    #endif

    Tk::Core::DebugUI::Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    Tk::Core::Graphics::ShaderManager::Shutdown();
    Tk::Core::Graphics::DestroyContext();
    Tk::Core::CoreFreeAligned(g_graphicsCommandStream.m_graphicsCommands);
    
    return 0;
}
