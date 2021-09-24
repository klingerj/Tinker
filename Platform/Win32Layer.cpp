#include "Core/CoreDefines.h"
#include "PlatformCommon.h"
#include "PlatformGameAPI.h"
#include "Win32Client.h"
#include "Win32WorkerThreadPool.cpp"
#include "Graphics/GraphicsCommon.h"
#include "ShaderManager.h"
#include "Core/Utility/Logging.h"
#include "Core/Utility/ScopedTimer.h"

#include <windows.h>
#include <Windowsx.h>

// TODO: make these to be compile defines
#define TINKER_PLATFORM_ENABLE_MULTITHREAD
#ifndef TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX
#define TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX 512
#endif
#ifndef TINKER_PLATFORM_HOTLOAD_FILENAME
#define TINKER_PLATFORM_HOTLOAD_FILENAME "TinkerGame_hotload.dll"
#endif

using namespace Tk;
using namespace Platform;

static GAME_UPDATE(GameUpdateStub) { return 0; }
static GAME_DESTROY(GameDestroyStub) {}
static GAME_WINDOW_RESIZE(GameWindowResizeStub) {}
typedef struct win32_game_code
{
    HMODULE GameDll = 0;
    FILETIME lastWriteTime = {};
    game_update* GameUpdate = GameUpdateStub;
    game_destroy* GameDestroy = GameDestroyStub;
    game_window_resize* GameWindowResize = GameWindowResizeStub;
} Win32GameCode;

Win32GameCode g_GameCode;
const char* GameDllStr = "TinkerGame.dll";

volatile bool runGame = true;

HWND g_windowHandle = NULL;
PlatformAPIFuncs g_platformAPIFuncs;
Graphics::GraphicsCommandStream g_graphicsCommandStream;
bool g_windowResized = false;

InputStateDeltas g_inputStateDeltas;

HCURSOR g_cursor = NULL;
bool g_cursorLocked = false;

#ifdef _SCRIPTS_DIR
#define SCRIPTS_PATH STRINGIFY(_SCRIPTS_DIR)
#else
//#define SCRIPTS_PATH "..\\Scripts\\"
#endif

#ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
WorkerThreadPool g_ThreadPool;
ENQUEUE_WORKER_THREAD_JOB(EnqueueWorkerThreadJob)
{
    g_ThreadPool.EnqueueNewThreadJob(newJob);
}
#else
ENQUEUE_WORKER_THREAD_JOB(EnqueueWorkerThreadJob)
{
    (*newJob)();
    newJob->m_done = true;
}
#endif

typedef struct global_app_params
{
    uint32 m_windowWidth;
    uint32 m_windowHeight;
} GlobalAppParams;
GlobalAppParams g_GlobalAppParams;

SYSTEM_INFO g_SystemInfo;
PlatformWindowHandles g_platformWindowHandles;

static bool ReloadGameCode(Win32GameCode* GameCode, const char* gameDllSourcePath)
{
    HANDLE gameDllFileHandle = CreateFile(gameDllSourcePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

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
            Core::Utility::LogMsg("Platform", "Failed to get handle to game dll to reload!", Core::Utility::LogSeverity::eCritical);
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
        Core::Utility::LogMsg("Platform", "Loading game dll!", Core::Utility::LogSeverity::eInfo);

        // Unload old code
        if (GameCode->GameDll)
        {
            GameCode->GameDestroy(&g_platformAPIFuncs);
            FreeLibrary(GameCode->GameDll);
            GameCode->GameUpdate = GameUpdateStub;
            GameCode->GameDestroy = GameDestroyStub;
            GameCode->GameWindowResize = GameWindowResizeStub;
        }

        CopyFile(gameDllSourcePath, TINKER_PLATFORM_HOTLOAD_FILENAME, FALSE);
        GameCode->GameDll = LoadLibrary(TINKER_PLATFORM_HOTLOAD_FILENAME);
        if (GameCode->GameDll)
        {
            GameCode->GameUpdate = (game_update*)GetProcAddress(GameCode->GameDll, "GameUpdate");
            GameCode->GameDestroy = (game_destroy*)GetProcAddress(GameCode->GameDll, "GameDestroy");
            GameCode->GameWindowResize = (game_window_resize*)GetProcAddress(GameCode->GameDll, "GameWindowResize");
            GameCode->lastWriteTime = gameDllLastWriteTime;
            return true; // reload successfully
        }
    }

    return false; // didn't reload
}

GET_FILE_SIZE(GetFileSize)
{
    HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    uint32 fileSize = 0;
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSizeInBytes = {};
        if (GetFileSizeEx(fileHandle, &fileSizeInBytes))
        {
            fileSize = SafeTruncateUint64(fileSizeInBytes.QuadPart);
        }
        else
        {
            fileSize = 0;
        }
        CloseHandle(fileHandle);
        return fileSize;
    }
    else
    {
        Core::Utility::LogMsg("Platform", "Unable to create file handle!", Core::Utility::LogSeverity::eCritical);
        return 0;
    }
}

READ_ENTIRE_FILE(ReadEntireFile)
{
    // User must specify a file size and the dest buffer.
    TINKER_ASSERT(fileSizeInBytes && buffer);

    HANDLE fileHandle = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        uint32 fileSize = 0;
        if (fileSizeInBytes)
        {
            fileSize = fileSizeInBytes;
        }
        else
        {
            fileSize = GetFileSize(filename);
        }

        DWORD numBytesRead = 0;
        ReadFile(fileHandle, buffer, fileSize, &numBytesRead, 0);
        TINKER_ASSERT(numBytesRead == fileSize);
        CloseHandle(fileHandle);
    }
    else
    {
        Core::Utility::LogMsg("Platform", "Unable to create file handle!", Core::Utility::LogSeverity::eCritical);
    }
}

WRITE_ENTIRE_FILE(WriteEntireFile)
{
    // User must specify a file size and the dest buffer.
    TINKER_ASSERT(fileSizeInBytes && buffer);

    HANDLE fileHandle = CreateFile(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD numBytesWritten = 0;
        WriteFile(fileHandle, buffer, fileSizeInBytes, &numBytesWritten, 0);
        TINKER_ASSERT(numBytesWritten == fileSizeInBytes);
        CloseHandle(fileHandle);
    }
    else
    {
        DWORD dw = GetLastError();
        Core::Utility::LogMsg("Platform", "Unable to create file handle!", Core::Utility::LogSeverity::eCritical);
    }
}

INIT_NETWORK_CONNECTION(InitNetworkConnection)
{
    if (Network::InitClient() != 0)
    {
        Core::Utility::LogMsg("Platform", "Failed to init network client!", Core::Utility::LogSeverity::eCritical);
        return 1;
    }
    else
    {
        Core::Utility::LogMsg("Platform", "Successfully initialized network client.", Core::Utility::LogSeverity::eInfo);
        return 0;
    }
}

END_NETWORK_CONNECTION(EndNetworkConnection)
{
    if (Network::DisconnectFromServer() != 0)
    {
        Core::Utility::LogMsg("Platform", "Failed to cleanup network client!", Core::Utility::LogSeverity::eCritical);
        return 1;
    }
    else
    {
        Core::Utility::LogMsg("Platform", "Successfully cleaned up network client.", Core::Utility::LogSeverity::eInfo);
        return 0;
    }
}

SEND_MESSAGE_TO_SERVER(SendMessageToServer)
{
    if (Network::SendMessageToServer() != 0)
    {
        Core::Utility::LogMsg("Platform", "Failed to send message to server!", Core::Utility::LogSeverity::eCritical);
        return 1;
    }
    else
    {
        return 0;
    }
}

SYSTEM_COMMAND(SystemCommand)
{
    STARTUPINFO startupInfo = {};
    PROCESS_INFORMATION processInfo = {};

    if (!CreateProcess(NULL, (LPSTR)command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startupInfo, &processInfo))
    {
        Core::Utility::LogMsg("Platform", "Failed to create new process to execute system command:", Core::Utility::LogSeverity::eCritical);
        Core::Utility::LogMsg("Platform", command, Core::Utility::LogSeverity::eCritical);
        return 1;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);

    DWORD exitCode;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);

    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return exitCode;
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
                Core::Utility::LogMsg("Platform", "Attempting to hotload shaders...\n", Core::Utility::LogSeverity::eInfo);
                const char* shaderCompileCommand = SCRIPTS_PATH "build_compile_shaders_glsl2spv.bat";
                if (SystemCommand(shaderCompileCommand) == 0)
                {
                    g_ShaderManager.ReloadShaders(&g_platformAPIFuncs, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
                    g_ShaderManager.LoadAllShaders(&g_platformAPIFuncs, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
                }
                else
                {
                    Core::Utility::LogMsg("Platform", "Failed to create shader compile process! Shaders will not be compiled.\n", Core::Utility::LogSeverity::eWarning);
                }
                Core::Utility::LogMsg("Platform", "...Done.\n", Core::Utility::LogSeverity::eInfo);
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
                Graphics::WindowMinimized();
            }
            else
            {
                // Normal window resize / maximize
                g_GlobalAppParams.m_windowWidth = LOWORD(lParam);
                g_GlobalAppParams.m_windowHeight = HIWORD(lParam);
                g_windowResized = true; // swap chain will be recreated later when this flag is checked
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
                Core::Utility::LogMsg("Platform", "Failed to change process priority when changing window focus!", Core::Utility::LogSeverity::eCritical);
            }
            else
            {
                Core::Utility::LogMsg("Platform", "Changing process priority!", Core::Utility::LogSeverity::eInfo);
                if (wParam)
                {
                    Core::Utility::LogMsg("Platform", "ABOVE_NORMAL", Core::Utility::LogSeverity::eInfo);
                }
                else
                {
                    Core::Utility::LogMsg("Platform", "NORMAL", Core::Utility::LogSeverity::eInfo);
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
            Core::Utility::LogMsg("Platform", "Failed to register window class!", Core::Utility::LogSeverity::eCritical);
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
            Core::Utility::LogMsg("Platform", "Failed to create window!", Core::Utility::LogSeverity::eCritical);
            return 1;
        }

        g_platformWindowHandles = {};
        g_platformWindowHandles.instance = hInstance;
        g_platformWindowHandles.windowHandle = g_windowHandle;
        Graphics::CreateContext(&g_platformWindowHandles, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
        
        g_platformAPIFuncs = {};
        g_platformAPIFuncs.EnqueueWorkerThreadJob  = EnqueueWorkerThreadJob;
        // Generic OS
        g_platformAPIFuncs.ReadEntireFile          = ReadEntireFile;
        g_platformAPIFuncs.WriteEntireFile         = WriteEntireFile;
        g_platformAPIFuncs.GetFileSize             = GetFileSize;
        g_platformAPIFuncs.SystemCommand           = SystemCommand;
        // Network
        g_platformAPIFuncs.InitNetworkConnection   = InitNetworkConnection;
        g_platformAPIFuncs.EndNetworkConnection    = EndNetworkConnection;
        g_platformAPIFuncs.SendMessageToServer     = SendMessageToServer;

        g_graphicsCommandStream = {};
        g_graphicsCommandStream.m_numCommands = 0;
        g_graphicsCommandStream.m_maxCommands = TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX;
        g_graphicsCommandStream.m_graphicsCommands = (Graphics::GraphicsCommand*)_aligned_malloc_dbg(g_graphicsCommandStream.m_maxCommands * sizeof(Graphics::GraphicsCommand), CACHE_LINE, __FILE__, __LINE__);

        #ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
        g_ThreadPool.Startup(g_SystemInfo.dwNumberOfProcessors / 2);
        #endif

        g_ShaderManager.Startup();
        g_ShaderManager.LoadAllShaderResources(&g_platformAPIFuncs, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);

        g_GameCode = {};
        bool reloaded = ReloadGameCode(&g_GameCode, GameDllStr);

        // Input handling
        g_inputStateDeltas = {};
        g_cursorLocked = true;
        ShowCursor(FALSE);
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

            bool shouldRenderFrame = false;
            {
                //TIMED_SCOPED_BLOCK("Window resize check");
                if (g_windowResized)
                {
                    shouldRenderFrame = Graphics::WindowResize();
                    g_ShaderManager.CreateWindowDependentResources(&g_platformAPIFuncs, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
                    g_GameCode.GameWindowResize(&g_platformAPIFuncs, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
                }
                g_windowResized = false;
            }

            if (shouldRenderFrame)
            {
                {
                    //TIMED_SCOPED_BLOCK("Acquire Frame");
                    Graphics::AcquireFrame();
                }

                {
                    //TIMED_SCOPED_BLOCK("Game Update");

                    int error = g_GameCode.GameUpdate(&g_platformAPIFuncs, &g_graphicsCommandStream, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight, &g_inputStateDeltas);
                    if (error != 0)
                    {
                        Core::Utility::LogMsg("Platform", "Error occurred in game code! Shutting down application.", Core::Utility::LogSeverity::eCritical);
                        runGame = false;
                        break;
                    }
                }

                // Process command stream
                {
                    //TIMED_SCOPED_BLOCK("Graphics command stream processing");
                    Graphics::BeginFrameRecording();
                    Graphics::ProcessGraphicsCommandStream(&g_graphicsCommandStream, false);
                    Graphics::EndFrameRecording();
                    Graphics::SubmitFrameToGPU();
                }
            }
        }

        if (ReloadGameCode(&g_GameCode, GameDllStr))
        {
            // Reset application resources
            #ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
            g_ThreadPool.Shutdown();
            g_ThreadPool.Startup(g_SystemInfo.dwNumberOfProcessors / 2);
            #endif

            Graphics::RecreateContext(&g_platformWindowHandles, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
        }
    }

    g_GameCode.GameDestroy(&g_platformAPIFuncs);

    #ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
    g_ThreadPool.Shutdown();
    #endif

    g_ShaderManager.Shutdown();
    Graphics::DestroyContext();

    _aligned_free(g_graphicsCommandStream.m_graphicsCommands);

    #if defined(MEM_TRACKING)
    _CrtDumpMemoryLeaks();
    #endif

    return 0;
}
