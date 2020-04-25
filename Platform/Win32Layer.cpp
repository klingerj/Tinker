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

int WINAPI
wWinMain(HINSTANCE hInstance,
         HINSTANCE hPrevInstance,
         PWSTR pCmdLine,
         int nCmdShow)
{
    AllocConsole();

    GameMemory GameMem = {};
    GameMem.EnqueueWorkerThreadJob = EnqueueWorkerThreadJob;

    for (;;)
    {
        Win32GameCode GameCode = {};
        ReloadGameCode(&GameCode);
        
        GameCode.GameUpdate(&GameMem);

        Sleep(2000); // TODO: remove once we add file timestamp checking
    }

    //return 0;
}
