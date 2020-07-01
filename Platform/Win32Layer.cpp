#include "../Include/PlatformGameAPI.h"
#include "../Include/Platform/Win32Vulkan.h"
#include "Win32WorkerThreadPool.cpp"

#include <windows.h>

using namespace Tinker;
using namespace Platform;

static GAME_UPDATE(GameUpdateStub) { return 0; }
typedef struct win32_game_code
{
    HMODULE GameDll = 0;
    FILETIME lastWriteTime = {};
    game_update *GameUpdate = GameUpdateStub;
} Win32GameCode;

static void ReloadGameCode(Win32GameCode* GameCode, const char* gameDllSourcePath)
{
    WIN32_FIND_DATA findData;
    HANDLE findHandle = FindFirstFile(gameDllSourcePath, &findData);
    if (findHandle != INVALID_HANDLE_VALUE)
    {
        FindClose(findHandle);
        if (CompareFileTime(&findData.ftLastWriteTime, &GameCode->lastWriteTime))
        {
            // Unload old code
            if (GameCode->GameDll)
            {
                FreeLibrary(GameCode->GameDll);
                GameCode->GameUpdate = GameUpdateStub;
            }

            const char* GameDllHotloadStr = "TinkerGame_hotload.dll";
            CopyFile(gameDllSourcePath, GameDllHotloadStr, FALSE);
            GameCode->GameDll = LoadLibrary(GameDllHotloadStr);
            if (GameCode->GameDll)
            {
                GameCode->GameUpdate = (game_update*)GetProcAddress(GameCode->GameDll, "GameUpdate");
                GameCode->lastWriteTime = findData.ftLastWriteTime;
            }
        }
    }
    else
    {
        // TODO: Log? Fail?
        // Game code does not get reloaded
    }
}

WorkerThreadPool g_ThreadPool;
ENQUEUE_WORKER_THREAD_JOB(EnqueueWorkerThreadJob)
{
    g_ThreadPool.EnqueueNewThreadJob(newJob);
}

static void ProcessGraphicsCommandStream(GraphicsCommandStream* graphicsCommandStream)
{
    TINKER_ASSERT(graphicsCommandStream->m_numCommands <= graphicsCommandStream->m_maxCommands);

    // TODO: sort commands, record command buffers on separate threads, etc
    for (uint32 i = 0; i < graphicsCommandStream->m_numCommands; ++i)
    {
        TINKER_ASSERT(graphicsCommandStream->m_graphicsCommands[i].m_commandType < eGraphicsCmdMax);
        switch (graphicsCommandStream->m_graphicsCommands[i].m_commandType)
        {
            case eGraphicsCmdDrawCall:
            {
                PrintDebugString("Issued a draw call!\n");
                // TODO: do api-specific graphics stuff
                break;
            }
            default:
            {
                // Invalid command type
                TINKER_ASSERT(false);
                break;
            }
        }
    }
}

volatile bool runGame = true;
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
            OutputDebugString("create\n");
            break;
        }
        case WM_SIZE:
        {
            OutputDebugString("size\n");
            break;
        }
        case WM_DESTROY:
        {
            OutputDebugString("destroy\n");
            break;
        }
        case WM_CLOSE:
        {
            OutputDebugString("close\n");
            PostQuitMessage(0);
            runGame = false;
            break;
        }
        case WM_ACTIVATEAPP:
        {
            OutputDebugString("activateapp\n");
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
    MSG msg = {};
    while (1)
    {
        BOOL Result = PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
        if (Result)
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
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
    // Setup window
    WNDCLASS windowClass = {};
    windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    //WindowClass.hIcon = ;
    windowClass.lpszClassName = "Tinker Platform Window";
    if (!RegisterClass(&windowClass))
    {
        // TODO: Log? Fail?
        return 1;
    }
    HWND windowHandle =
        CreateWindowEx(0,
                       windowClass.lpszClassName,
                       "Tinker",
                       WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       CW_USEDEFAULT,
                       0,
                       0,
                       hInstance,
                       0);
    if (!windowHandle)
    {
        // TODO: Log? Fail?
        return 1;
    }

    // Init Vulkan
    VulkanContextResources vulkanContextResources = {};
    // TODO: set desired window dims with settings file
    int result = InitVulkan(&vulkanContextResources, hInstance, windowHandle, 1920, 1080);
    if (result)
    {
        // TODO: Log? Fail?
        return 1;
    }

    // Init data passed from platform to game
    PlatformAPIFuncs platformAPIFuncs = {};
    platformAPIFuncs.EnqueueWorkerThreadJob = EnqueueWorkerThreadJob;

    GraphicsCommandStream graphicsCommandStream = {};
    graphicsCommandStream.m_numCommands = 0;
    graphicsCommandStream.m_maxCommands = 2; // TODO: set with compile flag
    graphicsCommandStream.m_graphicsCommands = new GraphicsCommand[graphicsCommandStream.m_maxCommands];

    Win32GameCode GameCode = {};
    const char* GameDllStr = "TinkerGame.dll";
    ReloadGameCode(&GameCode, GameDllStr);

    // Start threadpool
    g_ThreadPool.Startup(10);

    // Main loop
    while (runGame)
    {
        ProcessWindowMessages();
        
        GameCode.GameUpdate(&platformAPIFuncs, &graphicsCommandStream);

        ProcessGraphicsCommandStream(&graphicsCommandStream);

        ReloadGameCode(&GameCode, GameDllStr);
    }

    g_ThreadPool.Shutdown();
    DestroyVulkan(&vulkanContextResources);

    return 0;
}
