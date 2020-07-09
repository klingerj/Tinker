#include "../Include/PlatformGameAPI.h"
#include "../Include/Platform/Win32Utilities.h"
#include "../Include/Platform/Win32Vulkan.h"
#include "Win32WorkerThreadPool.cpp"

#include <windows.h>

using namespace Tinker;
using namespace Platform;

static GAME_UPDATE(GameUpdateStub) { return 0; }
static GAME_DESTROY(GameDestroyStub) {}
typedef struct win32_game_code
{
    HMODULE GameDll = 0;
    FILETIME lastWriteTime = {};
    game_update* GameUpdate = GameUpdateStub;
    game_destroy* GameDestroy = GameDestroyStub;
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
                GameCode->GameDestroy = GameDestroyStub;
            }

            const char* GameDllHotloadStr = "TinkerGame_hotload.dll";
            CopyFile(gameDllSourcePath, GameDllHotloadStr, FALSE);
            GameCode->GameDll = LoadLibrary(GameDllHotloadStr);
            if (GameCode->GameDll)
            {
                GameCode->GameUpdate = (game_update*)GetProcAddress(GameCode->GameDll, "GameUpdate");
                GameCode->GameDestroy = (game_destroy*)GetProcAddress(GameCode->GameDll, "GameDestroy");
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

Graphics::VulkanContextResources vulkanContextResources;

void BeginFrameRecording()
{
    // TODO: switch statement based on chosen graphics API
    BeginVulkanCommandRecording(&vulkanContextResources);
}

void EndFrameRecording()
{
    // TODO: switch statement based on chosen graphics API
    EndVulkanCommandRecording(&vulkanContextResources);
}

static void ProcessGraphicsCommandStream(GraphicsCommandStream* graphicsCommandStream)
{
    TINKER_ASSERT(graphicsCommandStream->m_numCommands <= graphicsCommandStream->m_maxCommands);

    BeginFrameRecording();

    for (uint32 i = 0; i < graphicsCommandStream->m_numCommands; ++i)
    {
        TINKER_ASSERT(graphicsCommandStream->m_graphicsCommands[i].m_commandType < eGraphicsCmdMax);

        const GraphicsCommand& currentCmd = graphicsCommandStream->m_graphicsCommands[i];
        switch (currentCmd.m_commandType)
        {
            case eGraphicsCmdDrawCall:
            {
                // TODO: switch statement based on chosen graphics API
                VulkanRecordCommandDrawCall(&vulkanContextResources,
                    currentCmd.m_vertexBufferHandle, currentCmd.m_indexBufferHandle,
                    currentCmd.m_numIndices, currentCmd.m_numVertices);
                break;
            }

            case eGraphicsCmdMemTransfer:
            {
                // TODO: switch statement based on chosen graphics API
                VulkanRecordCommandMemoryTransfer(&vulkanContextResources,
                    currentCmd.m_sizeInBytes, currentCmd.m_srcBufferHandle, currentCmd.m_dstBufferHandle);
                break;
            }

            case eGraphicsCmdRenderPassBegin:
            {
                VulkanRecordCommandRenderPassBegin(&vulkanContextResources, currentCmd.m_framebufferHandle,
                    currentCmd.m_renderWidth, currentCmd.m_renderHeight);
                break;
            }

            case eGraphicsCmdRenderPassEnd:
            {
                VulkanRecordCommandRenderPassEnd(&vulkanContextResources);
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

    EndFrameRecording();
}

static void SubmitFrameToGPU()
{
    // TODO: switch statement based on chosen graphics API
    SubmitFrame(&vulkanContextResources);
}

CREATE_VERTEX_BUFFER(CreateVertexBuffer)
{
    // TODO: switch statement based on chosen graphics API
    return CreateVertexBuffer(&vulkanContextResources, sizeInBytes, bufferType);
}

CREATE_STAGING_BUFFER(CreateStagingBuffer)
{
    // TODO: switch statement based on chosen graphics API
    Graphics::VulkanStagingBufferData vulkanData =
        CreateStagingBuffer(&vulkanContextResources, sizeInBytes);

    Graphics::StagingBufferData data;
    memcpy(&data, &vulkanData, sizeof(Graphics::VulkanStagingBufferData));
    return data;
}

CREATE_FRAMEBUFFER(CreateFramebuffer)
{
    // TODO: switch statement based on chosen graphics API
    return CreateFramebuffer(&vulkanContextResources,
        imageViewResourceHandles, numImageViewResourceHandles,
        width, height);
}

CREATE_IMAGE_RESOURCE(CreateImageResource)
{
    // TODO: switch statement based on chosen graphics API
    return CreateImageResource(&vulkanContextResources, width, height);
}

CREATE_IMAGE_VIEW_RESOURCE(CreateImageViewResource)
{
    // TODO: switch statement based on chosen graphics API
    return CreateImageViewResource(&vulkanContextResources, imageResourceHandle);
}

DESTROY_VERTEX_BUFFER(DestroyVertexBuffer)
{
    // TODO: switch statement based on chosen graphics API
    DestroyVertexBuffer(&vulkanContextResources, handle);
}

DESTROY_STAGING_BUFFER(DestroyStagingBuffer)
{
    // TODO: switch statement based on chosen graphics API
    DestroyStagingBuffer(&vulkanContextResources, handle);
}

DESTROY_FRAMEBUFFER(DestroyFramebuffer)
{
    // TODO: switch statement based on chosen graphics API
    DestroyFramebuffer(&vulkanContextResources, handle);
}

DESTROY_IMAGE_RESOURCE(DestroyImageResource)
{
    // TODO: switch statement based on chosen graphics API
    DestroyImageResource(&vulkanContextResources, handle);
}

DESTROY_IMAGE_VIEW_RESOURCE(DestroyImageViewResource)
{
    // TODO: switch statement based on chosen graphics API
    DestroyImageViewResource(&vulkanContextResources, handle);
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
            //OutputDebugString("size\n");
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
    // Get system info
    SYSTEM_INFO systemInfo;
    GetSystemInfo(&systemInfo);

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

    // TODO: load from settings file
    uint32 width = 800;
    uint32 height = 600;

    HWND windowHandle =
        CreateWindowEx(0,
            windowClass.lpszClassName,
            "Tinker",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            width,
            height,
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
    vulkanContextResources = {};
    // TODO: set desired window dims with settings file
    int result = InitVulkan(&vulkanContextResources, hInstance, windowHandle, width, height);
    if (result)
    {
        // TODO: Log? Fail?
        return 1;
    }

    // Init data passed from platform to game
    PlatformAPIFuncs platformAPIFuncs = {};
    platformAPIFuncs.EnqueueWorkerThreadJob = EnqueueWorkerThreadJob;
    platformAPIFuncs.ReadEntireFile = ReadEntireFile;
    platformAPIFuncs.CreateVertexBuffer = CreateVertexBuffer;
    platformAPIFuncs.CreateStagingBuffer = CreateStagingBuffer;
    platformAPIFuncs.DestroyVertexBuffer = DestroyVertexBuffer;
    platformAPIFuncs.DestroyStagingBuffer = DestroyStagingBuffer;
    platformAPIFuncs.CreateFramebuffer = CreateFramebuffer;
    platformAPIFuncs.DestroyFramebuffer = DestroyFramebuffer;
    platformAPIFuncs.CreateImageResource = CreateImageResource;
    platformAPIFuncs.DestroyImageResource = DestroyImageResource;
    platformAPIFuncs.CreateImageViewResource = CreateImageViewResource;
    platformAPIFuncs.DestroyImageViewResource = DestroyImageViewResource;

    GraphicsCommandStream graphicsCommandStream = {};
    graphicsCommandStream.m_numCommands = 0;
    graphicsCommandStream.m_maxCommands = 32; // TODO: set with compile flag
    graphicsCommandStream.m_graphicsCommands = new GraphicsCommand[graphicsCommandStream.m_maxCommands];

    Win32GameCode GameCode = {};
    const char* GameDllStr = "TinkerGame.dll";
    ReloadGameCode(&GameCode, GameDllStr);

    // Start threadpool
    uint32 numThreads = systemInfo.dwNumberOfProcessors;
    g_ThreadPool.Startup(numThreads / 2);

    // Main loop
    while (runGame)
    {
        ProcessWindowMessages();

        GameCode.GameUpdate(&platformAPIFuncs, &graphicsCommandStream);

        ProcessGraphicsCommandStream(&graphicsCommandStream);
        SubmitFrameToGPU();

        ReloadGameCode(&GameCode, GameDllStr);
    }

    GameCode.GameDestroy(&platformAPIFuncs);

    g_ThreadPool.Shutdown();
    DestroyVulkan(&vulkanContextResources);

    return 0;
}