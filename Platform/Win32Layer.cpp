#include "../Include/PlatformGameAPI.h"
#include "Win32WorkerThreadPool.cpp"

#include <windows.h>

using namespace Tinker;
using namespace Platform;

static GAME_UPDATE(GameUpdateStub) { return 0; }
typedef struct win32_game_code
{
    HMODULE GameDll = 0;
    game_update *GameUpdate = GameUpdateStub;
} Win32GameCode;

static void ReloadGameCode(Win32GameCode* GameCode)
{
    // Unload old code
    if (GameCode->GameDll)
    {
        FreeLibrary(GameCode->GameDll);
        GameCode->GameUpdate = GameUpdateStub;
    }

    // TODO: check file timestamp

    // Load new code
    const char* GameDllStr = "TinkerGame.dll";
    const char* GameDllHotloadStr = "TinkerGame_hotload.dll";

    CopyFile(GameDllStr, GameDllHotloadStr, FALSE);
    GameCode->GameDll = LoadLibraryA(GameDllHotloadStr);
    if (GameCode->GameDll)
    {
        GameCode->GameUpdate = (game_update *)GetProcAddress(GameCode->GameDll, "GameUpdate");
    }
}

WorkerThreadPool g_ThreadPool;
ENQUEUE_WORKER_THREAD_JOB(EnqueueWorkerThreadJob)
{
    g_ThreadPool.EnqueueNewThreadJob(newJob);
}
#include <assert.h>
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
            const char* msg = "Issued a draw call!\n";
            Print(msg, strlen(msg));
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

int WINAPI
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         PWSTR pCmdLine,
         int nCmdShow)
{
    AllocConsole();

    PlatformAPIFuncs platformAPIFuncs = {};
    platformAPIFuncs.EnqueueWorkerThreadJob = EnqueueWorkerThreadJob;

    GraphicsCommandStream graphicsCommandStream = {};
    graphicsCommandStream.m_numCommands = 0;
    graphicsCommandStream.m_maxCommands = 2; // TODO: set with compile flag
    graphicsCommandStream.m_graphicsCommands = new GraphicsCommand[graphicsCommandStream.m_maxCommands];

    Win32GameCode GameCode = {};

    // Start threadpool
    g_ThreadPool.Startup(10);

    volatile bool runGame = true;
    while (runGame)
    {
        ReloadGameCode(&GameCode);
        
        GameCode.GameUpdate(&platformAPIFuncs, &graphicsCommandStream);

        ProcessGraphicsCommandStream(&graphicsCommandStream);

        Sleep(2000); // TODO: remove once we add file timestamp checking
    }

    //g_ThreadPool.Shutdown();

    //return 0;
}
