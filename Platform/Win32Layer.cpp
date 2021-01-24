#include "../Include/PlatformGameAPI.h"
#include "../Include/Platform/Graphics/Vulkan.h"
#include "../Include/Core/Utilities/Logging.h"
#include "../Include/Core/Utilities/ScopedTimer.h"
#include "../Include/Platform/Win32Client.h"
#include "Win32WorkerThreadPool.cpp"

#include <windows.h>

// TODO: make these to be compile defines
#define TINKER_PLATFORM_ENABLE_MULTITHREAD
#ifndef TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX
#define TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX 512
#endif
#ifndef TINKER_PLATFORM_HOTLOAD_FILENAME
#define TINKER_PLATFORM_HOTLOAD_FILENAME "TinkerGame_hotload.dll"
#endif

using namespace Tinker;
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

volatile bool runGame = true;
PlatformAPIFuncs g_platformAPIFuncs;
GraphicsCommandStream g_graphicsCommandStream;
Win32GameCode g_GameCode;
const char* GameDllStr = "TinkerGame.dll";
InputStateDeltas g_inputStateDeltas;
Graphics::VulkanContextResources vulkanContextResources;
bool g_windowResized = false;

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

SYSTEM_INFO g_SystemInfo;
Graphics::PlatformWindowHandles g_platformWindowHandles;

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
            Utility::LogMsg("Platform", "Failed to get handle to game dll to reload!", Utility::eLogSeverityCritical);
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
        Utility::LogMsg("Platform", "Loading game dll!", Utility::eLogSeverityInfo);

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
        Utility::LogMsg("Platform", "Unable to create file handle!", Utility::eLogSeverityCritical);
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

        DWORD numBytesRead = {};
        ReadFile(fileHandle, buffer, fileSize, &numBytesRead, 0);
        TINKER_ASSERT(numBytesRead == fileSize);
        CloseHandle(fileHandle);
    }
    else
    {
        Utility::LogMsg("Platform", "Unable to create file handle!", Utility::eLogSeverityCritical);
    }
}

static void ProcessGraphicsCommandStream(GraphicsCommandStream* graphicsCommandStream, bool immediateSubmit)
{
    TINKER_ASSERT(graphicsCommandStream->m_numCommands <= graphicsCommandStream->m_maxCommands);

    for (uint32 i = 0; i < graphicsCommandStream->m_numCommands; ++i)
    {
        TINKER_ASSERT(graphicsCommandStream->m_graphicsCommands[i].m_commandType < eGraphicsCmdMax);

        const GraphicsCommand& currentCmd = graphicsCommandStream->m_graphicsCommands[i];
        ShaderHandle currentShader = DefaultShaderHandle_Invalid;

        switch (currentCmd.m_commandType)
        {
            case eGraphicsCmdDrawCall:
            {
                switch (g_GlobalAppParams.m_graphicsAPI)
                {
                    case eGraphicsAPIVulkan:
                    {
                        if (currentShader != currentCmd.m_shaderHandle)
                        {
                            Graphics::VulkanRecordCommandBindShader(&vulkanContextResources,
                                currentCmd.m_shaderHandle, &currentCmd.m_descriptors[0], immediateSubmit);
                            currentShader = currentCmd.m_shaderHandle;
                        }
                        Graphics::VulkanRecordCommandDrawCall(&vulkanContextResources,
                            currentCmd.m_positionBufferHandle, currentCmd.m_uvBufferHandle,
                            currentCmd.m_normalBufferHandle, currentCmd.m_indexBufferHandle, currentCmd.m_numIndices,
                            currentCmd.debugLabel, immediateSubmit);
                        break;
                    }

                    default:
                    {
                        Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
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
                        Graphics::VulkanRecordCommandMemoryTransfer(&vulkanContextResources,
                            currentCmd.m_sizeInBytes, currentCmd.m_srcBufferHandle, currentCmd.m_dstBufferHandle,
                            currentCmd.debugLabel, immediateSubmit);
                        break;
                    }

                    default:
                    {
                        Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
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
                        Graphics::VulkanRecordCommandRenderPassBegin(&vulkanContextResources, currentCmd.m_framebufferHandle,
                            currentCmd.m_renderWidth, currentCmd.m_renderHeight,
                            currentCmd.debugLabel, immediateSubmit);
                        break;
                    }

                    default:
                    {
                        Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
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
                        Graphics::VulkanRecordCommandRenderPassEnd(&vulkanContextResources, immediateSubmit);
                        break;
                    }

                    default:
                    {
                        Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
                        runGame = false;
                    }
                }

                break;
            }

            case eGraphicsCmdLayoutTransition:
            {
                switch (g_GlobalAppParams.m_graphicsAPI)
                {
                    case eGraphicsAPIVulkan:
                    {
                        Graphics::VulkanRecordCommandTransitionLayout(&vulkanContextResources, currentCmd.m_imageHandle,
                            currentCmd.m_startLayout, currentCmd.m_endLayout,
                            currentCmd.debugLabel, immediateSubmit);
                        break;
                    }

                    default:
                    {
                        Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
                        runGame = false;
                    }
                }

                break;
            }

            case eGraphicsCmdClearImage :
            {
                switch (g_GlobalAppParams.m_graphicsAPI)
                {
                    case eGraphicsAPIVulkan:
                    {
                        Graphics::VulkanRecordCommandClearImage(&vulkanContextResources, currentCmd.m_imageHandle,
                            currentCmd.m_clearValue, currentCmd.debugLabel, immediateSubmit);
                        break;
                    }

                    default:
                    {
                        Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
                        runGame = false;
                    }
                }

                break;
            }

            /*case eGraphicsCmdImageCopy:
            {
                TINKER_ASSERT(0); // Don't use this command rn
                switch (g_GlobalAppParams.m_graphicsAPI)
                {
                    case eGraphicsAPIVulkan:
                    {
                        Graphics::VulkanRecordCommandImageCopy(&vulkanContextResources,
                            currentCmd.m_srcImgHandle, currentCmd.m_dstImgHandle,
                            currentCmd.m_width, currentCmd.m_height, immediateSubmit);
                        break;
                    }

                    default:
                    {
                        Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
                        runGame = false;
                    }
                }

                break;
            }*/

            default:
            {
                // Invalid command type
                TINKER_ASSERT(false);
                break;
            }
        }
    }
}

SUBMIT_CMDS_IMMEDIATE(SubmitCmdsImmediate)
{
    Graphics::BeginVulkanCommandRecordingImmediate(&vulkanContextResources);
    ProcessGraphicsCommandStream(graphicsCommandStream, true);
    Graphics::EndVulkanCommandRecordingImmediate(&vulkanContextResources);
}

static void BeginFrameRecording()
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
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
        }
    }
}

static void EndFrameRecording()
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
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
        }
    }
}

static void SubmitFrameToGPU()
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::VulkanSubmitFrame(&vulkanContextResources);
            break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
        }
    }
}

CREATE_RESOURCE(CreateResource)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            return Graphics::VulkanCreateResource(&vulkanContextResources, resDesc);
            //break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;

            return DefaultResHandle_Invalid;
        }
    }
}

CREATE_FRAMEBUFFER(CreateFramebuffer)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            return Graphics::VulkanCreateFramebuffer(&vulkanContextResources,
                rtColorHandles, numRTColorHandles, rtDepthHandle,
                colorEndLayout, width, height);
            //break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
            return DefaultFramebufferHandle_Invalid;
        }
    }
}

CREATE_GRAPHICS_PIPELINE(CreateGraphicsPipeline)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            return Graphics::VulkanCreateGraphicsPipeline(&vulkanContextResources,
                    vertexShaderCode, numVertexShaderBytes, fragmentShaderCode,
                    numFragmentShaderBytes, blendState, depthState,
                    viewportWidth, viewportHeight, framebufferHandle, descriptorHandle);
            //break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
            return DefaultShaderHandle_Invalid;
        }
    }
}

CREATE_DESCRIPTOR(CreateDescriptor)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            return Graphics::VulkanCreateDescriptor(&vulkanContextResources, descLayout);
            //break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
            return DefaultDescHandle_Invalid;
            //break;
        }
    }
}

DESTROY_RESOURCE(DestroyResource)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::VulkanDestroyResource(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
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
            Graphics::VulkanDestroyFramebuffer(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
            break;
        }
    }
}

DESTROY_GRAPHICS_PIPELINE(DestroyGraphicsPipeline)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::VulkanDestroyGraphicsPipeline(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
            break;
        }
    }
}

DESTROY_DESCRIPTOR(DestroyDescriptor)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::VulkanDestroyDescriptor(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
            break;
        }
    }
}

DESTROY_ALL_DESCRIPTORS(DestroyAllDescriptors)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::VulkanDestroyAllDescriptors(&vulkanContextResources);
            break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
            break;
        }
    }
}

WRITE_DESCRIPTOR(WriteDescriptor)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::VulkanWriteDescriptor(&vulkanContextResources, descLayout, descSetHandle, descSetDataHandles);
            break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
            break;
        }
    }
}

MAP_RESOURCE(MapResource)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            return Graphics::VulkanMapResource(&vulkanContextResources, handle);
            //break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
            return nullptr;
            //break;
        }
    }
}

UNMAP_RESOURCE(UnmapResource)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            Graphics::VulkanUnmapResource(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            runGame = false;
            break;
        }
    }
}

INIT_NETWORK_CONNECTION(InitNetworkConnection)
{
    if (Network::InitClient() != 0)
    {
        Utility::LogMsg("Platform", "Failed to init network client!", Utility::eLogSeverityCritical);
        return 1;
    }
    else
    {
        Utility::LogMsg("Platform", "Successfully initialized network client.", Utility::eLogSeverityInfo);
        return 0;
    }
}

END_NETWORK_CONNECTION(EndNetworkConnection)
{
    if (Network::DisconnectFromServer() != 0)
    {
        Utility::LogMsg("Platform", "Failed to cleanup network client!", Utility::eLogSeverityCritical);
        return 1;
    }
    else
    {
        Utility::LogMsg("Platform", "Successfully cleaned up network client.", Utility::eLogSeverityInfo);
        return 0;
    }
}

SEND_MESSAGE_TO_SERVER(SendMessageToServer)
{
    if (Network::SendMessageToServer() != 0)
    {
        Utility::LogMsg("Platform", "Failed to send message to server!", Utility::eLogSeverityCritical);
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
        Utility::LogMsg("Platform", "Failed to create new process to execute system command:", Utility::eLogSeverityCritical);
        Utility::LogMsg("Platform", command, Utility::eLogSeverityCritical);
        return 1;
    }

    WaitForSingleObject(processInfo.hProcess, INFINITE);
    CloseHandle(processInfo.hProcess);
    CloseHandle(processInfo.hThread);

    return 0;
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
                            // Normal window resize / maximize
                            g_GlobalAppParams.m_windowWidth = LOWORD(lParam);
                            g_GlobalAppParams.m_windowHeight = HIWORD(lParam);
                            g_windowResized = true; // swap chain will be recreated
                        }
                    }
                    break;
                }

                default:
                {
                    Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
                    runGame = false;
                    break;
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
                Utility::LogMsg("Platform", "Failed to change process priority when changing window focus!", Utility::eLogSeverityCritical);
            }
            else
            {
                Utility::LogMsg("Platform", "Changing process priority!", Utility::eLogSeverityInfo);
                if (wParam)
                {
                    Utility::LogMsg("Platform", "ABOVE_NORMAL", Utility::eLogSeverityInfo);
                }
                else
                {
                    Utility::LogMsg("Platform", "NORMAL", Utility::eLogSeverityInfo);
                }
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

static void HandleKeypressInput(uint32 win32Keycode, uint64 win32Flags)
{
    uint8 wasDown = (win32Flags & (1 << 30)) == 1;
    uint8 isDown = (win32Flags & (1 << 31)) == 0;

    uint8 gameKeyCode = eMaxKeycodes;

    switch (win32Keycode)
    {
        case 'W':
        {
            gameKeyCode = eKeyW;
            break;
        }

        case 'A':
        {
            gameKeyCode = eKeyA;
            break;
        }

        case 'S':
        {
            gameKeyCode = eKeyS;
            break;
        }

        case 'D':
        {
            gameKeyCode = eKeyD;
            break;
        }

        case VK_F10:
        {
            gameKeyCode = eKeyF10;
            break;
        }

        case VK_F11:
        {
            gameKeyCode = eKeyF11;
            break;
        }

        default:
        {
            //TINKER_ASSERT(0);
            return;
            //break;
        }
    }
 
    g_inputStateDeltas.keyCodes[gameKeyCode].isDown = isDown;
    ++g_inputStateDeltas.keyCodes[gameKeyCode].numStateChanges;
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
        g_GlobalAppParams.m_graphicsAPI = eGraphicsAPIVulkan;
        g_GlobalAppParams.m_windowWidth = 800;
        g_GlobalAppParams.m_windowHeight = 600;

        // Get system info
        g_SystemInfo = {};
        GetSystemInfo(&g_SystemInfo);

        // Setup window
        WNDCLASS windowClass = {};
        windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = hInstance;
        //WindowClass.hIcon = ;
        windowClass.lpszClassName = "Tinker Platform Window";
        if (!RegisterClass(&windowClass))
        {
            Utility::LogMsg("Platform", "Failed to register window class!", Utility::eLogSeverityCritical);
            return 1;
        }

        RECT windowDims = { 0, 0, (LONG)g_GlobalAppParams.m_windowWidth, (LONG)g_GlobalAppParams.m_windowHeight };
        AdjustWindowRect(&windowDims, WS_OVERLAPPEDWINDOW | WS_VISIBLE, FALSE);

        HWND windowHandle =
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

        if (!windowHandle)
        {
            Utility::LogMsg("Platform", "Failed to create window!", Utility::eLogSeverityCritical);
            return 1;
        }

        g_platformWindowHandles = {};
        g_platformWindowHandles.instance = hInstance;
        g_platformWindowHandles.windowHandle = windowHandle;

        switch (g_GlobalAppParams.m_graphicsAPI)
        {
            case eGraphicsAPIVulkan:
            {
                vulkanContextResources = {};
                int result = InitVulkan(&vulkanContextResources, &g_platformWindowHandles, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
                if (result)
                {
                    Utility::LogMsg("Platform", "Failed to init graphics backend!", Utility::eLogSeverityCritical);
                    return 1;
                }
                break;
            }

            default:
            {
                Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
                return 1;
            }
        }

        g_platformAPIFuncs = {};
        g_platformAPIFuncs.EnqueueWorkerThreadJob = EnqueueWorkerThreadJob;
        g_platformAPIFuncs.WaitOnThreadJob = WaitOnJob;
        g_platformAPIFuncs.ReadEntireFile = ReadEntireFile;
        g_platformAPIFuncs.GetFileSize = GetFileSize;
        g_platformAPIFuncs.InitNetworkConnection = InitNetworkConnection;
        g_platformAPIFuncs.EndNetworkConnection = EndNetworkConnection;
        g_platformAPIFuncs.SendMessageToServer = SendMessageToServer;
        g_platformAPIFuncs.SystemCommand = SystemCommand;
        // Graphics
        g_platformAPIFuncs.CreateResource = CreateResource;
        g_platformAPIFuncs.DestroyResource = DestroyResource;
        g_platformAPIFuncs.MapResource = MapResource;
        g_platformAPIFuncs.UnmapResource = UnmapResource;
        g_platformAPIFuncs.CreateFramebuffer = CreateFramebuffer;
        g_platformAPIFuncs.DestroyFramebuffer = DestroyFramebuffer;
        g_platformAPIFuncs.CreateGraphicsPipeline = CreateGraphicsPipeline;
        g_platformAPIFuncs.DestroyGraphicsPipeline = DestroyGraphicsPipeline;
        g_platformAPIFuncs.CreateDescriptor = CreateDescriptor;
        g_platformAPIFuncs.DestroyAllDescriptors = DestroyAllDescriptors;
        g_platformAPIFuncs.DestroyDescriptor = DestroyDescriptor;
        g_platformAPIFuncs.WriteDescriptor = WriteDescriptor;
        g_platformAPIFuncs.SubmitCmdsImmediate = SubmitCmdsImmediate;

        g_graphicsCommandStream = {};
        g_graphicsCommandStream.m_numCommands = 0;
        g_graphicsCommandStream.m_maxCommands = TINKER_PLATFORM_GRAPHICS_COMMAND_STREAM_MAX;
        g_graphicsCommandStream.m_graphicsCommands = (GraphicsCommand*)_aligned_malloc_dbg(g_graphicsCommandStream.m_maxCommands * sizeof(GraphicsCommand), 64, __FILE__, __LINE__);

        #ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
        g_ThreadPool.Startup(g_SystemInfo.dwNumberOfProcessors / 2);
        #endif

        g_GameCode = {};
        bool reloaded = ReloadGameCode(&g_GameCode, GameDllStr);

        g_inputStateDeltas = {};
    }

    // Main loop
    while (runGame)
    {
        {
            TIMED_SCOPED_BLOCK("-----> Total Frame");

            {
                //TIMED_SCOPED_BLOCK("Process window messages");

                ProcessWindowMessages();
            }

            bool shouldRenderFrame = false;
            {
                //TIMED_SCOPED_BLOCK("Window resize check");

                switch (g_GlobalAppParams.m_graphicsAPI)
                {
                    case eGraphicsAPIVulkan:
                    {
                        shouldRenderFrame = vulkanContextResources.isSwapChainValid;

                        if (g_windowResized)
                        {
                            VulkanDestroySwapChain(&vulkanContextResources);
                            VulkanCreateSwapChain(&vulkanContextResources);

                            g_GameCode.GameWindowResize(&g_platformAPIFuncs, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);

                            g_windowResized = false;
                        }

                        break;
                    }

                    default:
                    {
                        Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
                        shouldRenderFrame = false;
                        runGame = false;
                        break;
                    }
                }
            }

            if (shouldRenderFrame)
            {
                {
                    //TIMED_SCOPED_BLOCK("Acquire Frame");

                    AcquireFrame(&vulkanContextResources);
                }

                {
                    TIMED_SCOPED_BLOCK("Game Update");

                    int error = g_GameCode.GameUpdate(&g_platformAPIFuncs, &g_graphicsCommandStream, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight, &g_inputStateDeltas);
                    if (error != 0)
                    {
                        Utility::LogMsg("Platform", "Error occurred in game code! Shutting down application.", Utility::eLogSeverityCritical);
                        runGame = false;
                        break;
                    }
                }

                // Process command stream
                {
                    TIMED_SCOPED_BLOCK("Graphics command stream processing");
                    BeginFrameRecording();
                    ProcessGraphicsCommandStream(&g_graphicsCommandStream, false);
                    EndFrameRecording();
                    SubmitFrameToGPU();
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

            // Reset graphics context
            switch (g_GlobalAppParams.m_graphicsAPI)
            {
                case eGraphicsAPIVulkan:
                {
                    DestroyVulkan(&vulkanContextResources);
                    InitVulkan(&vulkanContextResources, &g_platformWindowHandles, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);
                    break;
                }

                default:
                {
                    Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
                    break;
                }
            }
        }
    }

    g_GameCode.GameDestroy(&g_platformAPIFuncs);

    #ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
    g_ThreadPool.Shutdown();
    #endif

    _aligned_free(g_graphicsCommandStream.m_graphicsCommands);

    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case eGraphicsAPIVulkan:
        {
            DestroyVulkan(&vulkanContextResources);
            break;
        }

        default:
        {
            Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Utility::eLogSeverityCritical);
            break;
        }
    }

    #if defined(MEM_TRACKING)
    _CrtDumpMemoryLeaks();
    #endif

    return 0;
}
