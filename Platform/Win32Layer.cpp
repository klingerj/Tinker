#include "../Include/PlatformGameAPI.h"
#include "../Include/Platform/Win32Utilities.h"
#include "../Include/Platform/Win32Vulkan.h"
#include "../Include/Platform/Win32Logs.h"
#include "../Include/Platform/Win32Client.h"
#include "Win32WorkerThreadPool.cpp"

#include <windows.h>

// TODO: make these to be compile defines
#define TINKER_PLATFORM_ENABLE_MULTITHREAD
#ifndef TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX
#define TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX 64
#endif
#ifndef TINKER_PLATFORM_HOTLOAD_FILENAME
#define TINKER_PLATFORM_HOTLOAD_FILENAME "TinkerGame_hotload.dll"
#endif

using namespace Tinker;
using namespace Platform;

volatile bool runGame = true;

// TODO: implement other graphics APIs
enum
{
    eGraphicsAPIVulkan = 0,
    eGraphicsAPIInvalid
};

typedef struct global_app_params
{
    uint8 m_graphicsAPI;
    uint32 m_windowWidth;
    uint32 m_windowHeight;
} GlobalAppParams;
GlobalAppParams g_GlobalAppParams;

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

            CopyFile(gameDllSourcePath, TINKER_PLATFORM_HOTLOAD_FILENAME, FALSE);
            GameCode->GameDll = LoadLibrary(TINKER_PLATFORM_HOTLOAD_FILENAME);
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
        // Game code does not get reloaded
        LogMsg("Failed to find game dll to reload!", eLogSeverityCritical);
    }
}

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

Graphics::VulkanContextResources vulkanContextResources;

void BeginFrameRecording()
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            BeginVulkanCommandRecording(&vulkanContextResources);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
        }
    }
}

void EndFrameRecording()
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            EndVulkanCommandRecording(&vulkanContextResources);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
        }
    }
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
                switch (g_GlobalAppParams.m_graphicsAPI)
                {
                    case eGraphicsAPIVulkan:
                    {
                        VulkanRecordCommandDrawCall(&vulkanContextResources,
                            currentCmd.m_vertexBufferHandle, currentCmd.m_indexBufferHandle,
                            currentCmd.m_numIndices, currentCmd.m_numVertices);
                        break;
                    }

                    default:
                    {
                        LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
                        runGame = false;
                    }
                }

                break;
            }

            case eGraphicsCmdMemTransfer:
            {
                switch (g_GlobalAppParams.m_graphicsAPI)
                {
                    case eGraphicsAPIVulkan:
                    {
                        VulkanRecordCommandMemoryTransfer(&vulkanContextResources,
                            currentCmd.m_sizeInBytes, currentCmd.m_srcBufferHandle, currentCmd.m_dstBufferHandle);
                        break;
                    }

                    default:
                    {
                        LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
                        runGame = false;
                    }
                }

                break;
            }

            case eGraphicsCmdRenderPassBegin:
            {
                switch (g_GlobalAppParams.m_graphicsAPI)
                {
                    case eGraphicsAPIVulkan:
                    {
                        VulkanRecordCommandRenderPassBegin(&vulkanContextResources, currentCmd.m_framebufferHandle,
                            currentCmd.m_renderWidth, currentCmd.m_renderHeight);
                        break;
                    }

                    default:
                    {
                        LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
                        runGame = false;
                    }
                }

                break;
            }

            case eGraphicsCmdRenderPassEnd:
            {
                switch (g_GlobalAppParams.m_graphicsAPI)
                {
                    case eGraphicsAPIVulkan:
                    {
                        VulkanRecordCommandRenderPassEnd(&vulkanContextResources);
                        break;
                    }

                    default:
                    {
                        LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
                        runGame = false;
                    }
                }

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
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::SubmitFrame(&vulkanContextResources);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
        }
    }
}

CREATE_VERTEX_BUFFER(CreateVertexBuffer)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            return Graphics::CreateVertexBuffer(&vulkanContextResources, sizeInBytes, bufferType);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
            return TINKER_INVALID_HANDLE;
        }
    }
}

CREATE_STAGING_BUFFER(CreateStagingBuffer)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::VulkanStagingBufferData vulkanData =
                Graphics::CreateStagingBuffer(&vulkanContextResources, sizeInBytes);

            Graphics::StagingBufferData data;
            memcpy(&data, &vulkanData, sizeof(Graphics::VulkanStagingBufferData));
            return data;
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
            Graphics::StagingBufferData data = {};
            return data;
        }
    }
}

CREATE_FRAMEBUFFER(CreateFramebuffer)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            return Graphics::CreateFramebuffer(&vulkanContextResources,
                imageViewResourceHandles, numImageViewResourceHandles,
                width, height);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
            return TINKER_INVALID_HANDLE;
        }
    }
}

CREATE_IMAGE_RESOURCE(CreateImageResource)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            return Graphics::CreateImageResource(&vulkanContextResources, width, height);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
            return TINKER_INVALID_HANDLE;
        }
    }
}

CREATE_IMAGE_VIEW_RESOURCE(CreateImageViewResource)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            return Graphics::CreateImageViewResource(&vulkanContextResources, imageResourceHandle);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
            return TINKER_INVALID_HANDLE;
        }
    }
}

DESTROY_VERTEX_BUFFER(DestroyVertexBuffer)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::DestroyVertexBuffer(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
            break;
        }
    }
}

DESTROY_STAGING_BUFFER(DestroyStagingBuffer)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::DestroyStagingBuffer(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
            break;
        }
    }
}

DESTROY_FRAMEBUFFER(DestroyFramebuffer)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::DestroyFramebuffer(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
            break;
        }
    }
}

DESTROY_IMAGE_RESOURCE(DestroyImageResource)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::DestroyImageResource(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
            break;
        }
    }
}

DESTROY_IMAGE_VIEW_RESOURCE(DestroyImageViewResource)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::DestroyImageViewResource(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            runGame = false;
            break;
        }
    }
}

INIT_NETWORK_CONNECTION(InitNetworkConnection)
{
    if (Network::InitClient() != 0)
    {
        LogMsg("Failed to init network client!", eLogSeverityCritical);
        return 1;
    }
    else
    {
        LogMsg("Successfully initialized network client.", eLogSeverityInfo);
        return 0;
    }
}

SEND_MESSAGE_TO_SERVER(SendMessageToServer)
{
    if (Network::SendMessageToServer() != 0)
    {
        LogMsg("Failed to send message to server!", eLogSeverityCritical);
        return 1;
    }
    else
    {
        return 0;
    }
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
            //OutputDebugString("create\n");
            break;
        }
        case WM_SIZE:
        {
            switch (g_GlobalAppParams.m_graphicsAPI)
            {
                case eGraphicsAPIVulkan:
                {
                    if (vulkanContextResources.isInitted)
                    {
                        if (wParam == 1) // window minimized - can't create swap chain with size 0
                        {
                            vulkanContextResources.isSwapChainValid = false;
                        }
                        else
                        {
                            DestroySwapChain(&vulkanContextResources);
                            CreateSwapChain(&vulkanContextResources);
                        }
                    }
                    break;
                }

                default:
                {
                    LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
                    runGame = false;
                    break;
                }
            }

            break;
        }
        case WM_DESTROY:
        {
            //OutputDebugString("destroy\n");
            break;
        }
        case WM_CLOSE:
        {
            //OutputDebugString("close\n");
            PostQuitMessage(0);
            runGame = false;
            break;
        }
        case WM_ACTIVATEAPP:
        {
            //OutputDebugString("activateapp\n");
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
    // TODO: load from settings file
    g_GlobalAppParams = {};
    g_GlobalAppParams.m_graphicsAPI = eGraphicsAPIVulkan;
    g_GlobalAppParams.m_windowWidth = 800;
    g_GlobalAppParams.m_windowHeight = 600;

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
        LogMsg("Failed to register window class!", eLogSeverityCritical);
        return 1;
    }

    HWND windowHandle =
        CreateWindowEx(0,
            windowClass.lpszClassName,
            "Tinker",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            g_GlobalAppParams.m_windowWidth,
            g_GlobalAppParams.m_windowHeight,
            0,
            0,
            hInstance,
            0);
    if (!windowHandle)
    {
        LogMsg("Failed to create window!", eLogSeverityCritical);
        return 1;
    }

    switch(g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            vulkanContextResources = {};
            int result = InitVulkan(&vulkanContextResources, hInstance, windowHandle, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
            if (result)
            {
                LogMsg("Failed to init graphics backend!", eLogSeverityCritical);
                return 1;
            }
            break;
        }

        default:
        {
            LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
            return 1;
        }
    }

    // Init data passed from platform to game
    PlatformAPIFuncs platformAPIFuncs = {};
    platformAPIFuncs.EnqueueWorkerThreadJob = EnqueueWorkerThreadJob;
    platformAPIFuncs.WaitOnThreadJob = WaitOnJob;
    platformAPIFuncs.ReadEntireFile = ReadEntireFile;
    platformAPIFuncs.InitNetworkConnection = InitNetworkConnection;
    platformAPIFuncs.SendMessageToServer = SendMessageToServer;
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
    graphicsCommandStream.m_maxCommands = TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX;
    graphicsCommandStream.m_graphicsCommands = new GraphicsCommand[graphicsCommandStream.m_maxCommands];

    Win32GameCode GameCode = {};
    const char* GameDllStr = "TinkerGame.dll";
    ReloadGameCode(&GameCode, GameDllStr);

    // Start threadpool
    #ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
    uint32 numThreads = systemInfo.dwNumberOfProcessors;
    g_ThreadPool.Startup(numThreads / 2);
    #endif

    // Main loop
    while (runGame)
    {
        ProcessWindowMessages();

        bool shouldRenderFrame = false;

        switch (g_GlobalAppParams.m_graphicsAPI)
        {
            case eGraphicsAPIVulkan:
            {
                shouldRenderFrame = vulkanContextResources.isSwapChainValid;
                break;
            }

            default:
            {
                LogMsg("Invalid/unsupported graphics API chosen!", eLogSeverityCritical);
                shouldRenderFrame = false;
                runGame = false;
                break;
            }
        }

        if (shouldRenderFrame)
        {
            int error = GameCode.GameUpdate(&platformAPIFuncs, &graphicsCommandStream);
            if (error != 0)
            {
                LogMsg("Error occurred in game code! Shutting down application.", eLogSeverityCritical);
                runGame = false;
                break;
            }
            ProcessGraphicsCommandStream(&graphicsCommandStream);
            SubmitFrameToGPU();
        }

        ReloadGameCode(&GameCode, GameDllStr);
    }

    GameCode.GameDestroy(&platformAPIFuncs);

    Network::DisconnectFromServer();

    #ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
    g_ThreadPool.Shutdown();
    #endif

    DestroyVulkan(&vulkanContextResources);

    return 0;
}
