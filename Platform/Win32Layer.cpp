#include "../Include/PlatformGameAPI.h"
#include "Win32WorkerThreadPool.cpp"

#include <windows.h>

using namespace Tinker;
using namespace Platform;

static GAME_UPDATE(GameUpdateStub) { return 0; }
typedef struct win32_game_code
{
    HMODULE GameDll;
    game_update *GameUpdate = GameUpdateStub;
} Win32GameCode;

static void LoadGameCode(Win32GameCode* GameCode)
{
    const char* GameDllStr = "TinkerGame.dll";
    const char* GameDllHotloadStr = "TinkerGame_hotload.dll";

    CopyFile(GameDllStr, GameDllHotloadStr, FALSE);
    GameCode->GameDll = LoadLibraryA(GameDllHotloadStr);
    if (GameCode->GameDll)
    {
        GameCode->GameUpdate = (game_update *)GetProcAddress(GameCode->GameDll, "GameUpdate");
    }
}

static void UnloadGameCode(Win32GameCode* GameCode)
{
    if (GameCode->GameDll)
    {
        FreeLibrary(GameCode->GameDll);
    }
    GameCode->GameUpdate = GameUpdateStub;
}

WorkerThreadPool g_ThreadPool;
ENQUEUE_WORKER_THREAD_JOB(EnqueueWorkerThreadJob)
{
    g_ThreadPool.EnqueueNewThreadJob(newJob);
}

// TODO: replace with winmain entry point
int main()
{
    GameMemory GameMem = {};
    GameMem.EnqueueWorkerThreadJob = EnqueueWorkerThreadJob;

    for (;;)
    {
        Win32GameCode GameCode = {};
        LoadGameCode(&GameCode);
        
        GameCode.GameUpdate(&GameMem);

        UnloadGameCode(&GameCode);
        Sleep(2000); // TODO: remove
    }

    //return 0;
}
