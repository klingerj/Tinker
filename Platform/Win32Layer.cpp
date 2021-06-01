#include "PlatformGameAPI.h"
#include "Platform/Graphics/Vulkan.h"
#include "Core/Utility/Logging.h"
#include "Core/Utility/ScopedTimer.h"
#include "Platform/Win32Client.h"
#include "Win32WorkerThreadPool.cpp"

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

volatile bool runGame = true;
HWND g_windowHandle = NULL;
HCURSOR g_cursor = NULL;
PlatformAPIFuncs g_platformAPIFuncs;
GraphicsCommandStream g_graphicsCommandStream;
Win32GameCode g_GameCode;
const char* GameDllStr = "TinkerGame.dll";
InputStateDeltas g_inputStateDeltas;
Graphics::VulkanContextResources vulkanContextResources;
bool g_windowResized = false;
bool g_cursorLocked = false;

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
enum GraphicsAPI
{
    eVulkan = 0,
    eInvalid
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

static void ProcessGraphicsCommandStream(GraphicsCommandStream* graphicsCommandStream, bool immediateSubmit)
{
    TINKER_ASSERT(graphicsCommandStream->m_numCommands <= graphicsCommandStream->m_maxCommands);

    const bool multithreadedCmdRecording = false;

    if (multithreadedCmdRecording)
    {
        // TODO: old API, change to be job list
        /*uint32 numThreads = g_SystemInfo.dwNumberOfProcessors / 2;
        Platform::WorkerJob** jobs = new Platform::WorkerJob*[numThreads];

        for (uint32 uiJob = 0; uiJob < g_SystemInfo.dwNumberOfProcessors / 2; ++uiJob)
        {
            // TODO: we could probably just simply break up the list of commands into even-ish chunks, record them,
            // and then plop them into the primary one
            // TODO: going to need to ask vulkan to give/specify a secondary buffer based on the thread id
            jobs[uiJob] = Platform::CreateNewThreadJob([=]()
                {
                });

            //platformFuncs->EnqueueWorkerThreadJob(jobs[uiJob]);
        }
        for (uint32 i = 0; i < numThreads; ++i)
        {
            WaitOnJob(jobs[i]);
        }
        for (uint32 i = 0; i < numThreads; ++i)
        {
            delete jobs[i];
        }
        delete jobs;*/
    }
    else
    {
        // Track number of instances for proper indexing into uniform buffer of instance data
        uint32 instanceCount = 0;

        for (uint32 i = 0; i < graphicsCommandStream->m_numCommands; ++i)
        {
            TINKER_ASSERT(graphicsCommandStream->m_graphicsCommands[i].m_commandType < GraphicsCmd::eMax);

            const GraphicsCommand& currentCmd = graphicsCommandStream->m_graphicsCommands[i];
            ShaderHandle currentShader = DefaultShaderHandle_Invalid;

            switch (currentCmd.m_commandType)
            {
                case GraphicsCmd::eDrawCall:
                {
                    switch (g_GlobalAppParams.m_graphicsAPI)
                    {
                        case GraphicsAPI::eVulkan:
                        {
                            if (currentShader != currentCmd.m_shaderHandle)
                            {
                                Graphics::VulkanRecordCommandBindShader(&vulkanContextResources,
                                    currentCmd.m_shaderHandle, &currentCmd.m_descriptors[0], immediateSubmit);
                                currentShader = currentCmd.m_shaderHandle;
                            }
                            {
                                uint32 data[4] = {};
                                data[0] = instanceCount;
                                Graphics::VulkanRecordCommandPushConstant(&vulkanContextResources, (uint8*)data, sizeof(uint32) * 4, currentShader);
                            }
                            Graphics::VulkanRecordCommandDrawCall(&vulkanContextResources,
                                currentCmd.m_positionBufferHandle, currentCmd.m_uvBufferHandle,
                                currentCmd.m_normalBufferHandle, currentCmd.m_indexBufferHandle, currentCmd.m_numIndices,
                                currentCmd.m_numInstances, currentCmd.debugLabel, immediateSubmit);
                            instanceCount += currentCmd.m_numInstances;
                            break;
                        }

                        default:
                        {
                            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
                            runGame = false;
                        }
                    }

                    break;
                }

                case GraphicsCmd::eMemTransfer:
                {
                    switch (g_GlobalAppParams.m_graphicsAPI)
                    {
                        case GraphicsAPI::eVulkan:
                        {
                            Graphics::VulkanRecordCommandMemoryTransfer(&vulkanContextResources,
                                currentCmd.m_sizeInBytes, currentCmd.m_srcBufferHandle, currentCmd.m_dstBufferHandle,
                                currentCmd.debugLabel, immediateSubmit);
                            break;
                        }

                        default:
                        {
                            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
                            runGame = false;
                        }
                    }

                    break;
                }

                case GraphicsCmd::eRenderPassBegin:
                {
                    switch (g_GlobalAppParams.m_graphicsAPI)
                    {
                        case GraphicsAPI::eVulkan:
                        {
                            instanceCount = 0;

                            Graphics::VulkanRecordCommandRenderPassBegin(&vulkanContextResources, currentCmd.m_framebufferHandle,
                                currentCmd.m_renderWidth, currentCmd.m_renderHeight,
                                currentCmd.debugLabel, immediateSubmit);
                            break;
                        }

                        default:
                        {
                            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
                            runGame = false;
                        }
                    }

                    break;
                }

                case GraphicsCmd::eRenderPassEnd:
                {
                    switch (g_GlobalAppParams.m_graphicsAPI)
                    {
                        case GraphicsAPI::eVulkan:
                        {
                            Graphics::VulkanRecordCommandRenderPassEnd(&vulkanContextResources, immediateSubmit);
                            break;
                        }

                        default:
                        {
                            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
                            runGame = false;
                        }
                    }

                    break;
                }

                case GraphicsCmd::eLayoutTransition:
                {
                    switch (g_GlobalAppParams.m_graphicsAPI)
                    {
                        case GraphicsAPI::eVulkan:
                        {
                            Graphics::VulkanRecordCommandTransitionLayout(&vulkanContextResources, currentCmd.m_imageHandle,
                                currentCmd.m_startLayout, currentCmd.m_endLayout,
                                currentCmd.debugLabel, immediateSubmit);
                            break;
                        }

                        default:
                        {
                            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
                            runGame = false;
                        }
                    }

                    break;
                }

                case GraphicsCmd::eClearImage:
                {
                    switch (g_GlobalAppParams.m_graphicsAPI)
                    {
                        case GraphicsAPI::eVulkan:
                        {
                            Graphics::VulkanRecordCommandClearImage(&vulkanContextResources, currentCmd.m_imageHandle,
                                currentCmd.m_clearValue, currentCmd.debugLabel, immediateSubmit);
                            break;
                        }

                        default:
                        {
                            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
                            runGame = false;
                        }
                    }

                    break;
                }

                default:
                {
                    // Invalid command type
                    TINKER_ASSERT(0);
                    break;
                }
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
        case GraphicsAPI::eVulkan:
        {
            BeginVulkanCommandRecording(&vulkanContextResources);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
        }
    }
}

static void EndFrameRecording()
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            EndVulkanCommandRecording(&vulkanContextResources);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
        }
    }
}

static void SubmitFrameToGPU()
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            Graphics::VulkanSubmitFrame(&vulkanContextResources);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
        }
    }
}

CREATE_RESOURCE(CreateResource)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            return Graphics::VulkanCreateResource(&vulkanContextResources, resDesc);
            //break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;

            return DefaultResHandle_Invalid;
        }
    }
}

CREATE_FRAMEBUFFER(CreateFramebuffer)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            return Graphics::VulkanCreateFramebuffer(&vulkanContextResources,
                rtColorHandles, numRTColorHandles, rtDepthHandle,
                colorEndLayout, width, height);
            //break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
            return DefaultFramebufferHandle_Invalid;
        }
    }
}

CREATE_GRAPHICS_PIPELINE(CreateGraphicsPipeline)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            return Graphics::VulkanCreateGraphicsPipeline(&vulkanContextResources,
                    vertexShaderCode, numVertexShaderBytes, fragmentShaderCode,
                    numFragmentShaderBytes, blendState, depthState,
                    viewportWidth, viewportHeight, framebufferHandle, descriptorHandles, numDescriptorHandles);
            //break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
            return DefaultShaderHandle_Invalid;
        }
    }
}

CREATE_DESCRIPTOR(CreateDescriptor)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            return Graphics::VulkanCreateDescriptor(&vulkanContextResources, descLayout);
            //break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
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
        case GraphicsAPI::eVulkan:
        {
            Graphics::VulkanDestroyResource(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
            break;
        }
    }
}

DESTROY_FRAMEBUFFER(DestroyFramebuffer)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            Graphics::VulkanDestroyFramebuffer(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
            break;
        }
    }
}

DESTROY_GRAPHICS_PIPELINE(DestroyGraphicsPipeline)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            Graphics::VulkanDestroyGraphicsPipeline(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
            break;
        }
    }
}

DESTROY_DESCRIPTOR(DestroyDescriptor)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            Graphics::VulkanDestroyDescriptor(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
            break;
        }
    }
}

DESTROY_ALL_DESCRIPTORS(DestroyAllDescriptors)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            Graphics::VulkanDestroyAllDescriptors(&vulkanContextResources);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
            break;
        }
    }
}

WRITE_DESCRIPTOR(WriteDescriptor)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            Graphics::VulkanWriteDescriptor(&vulkanContextResources, descLayout, descSetHandles, descSetDataHandles);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
            break;
        }
    }
}

MAP_RESOURCE(MapResource)
{
    switch (g_GlobalAppParams.m_graphicsAPI)
    {
        case GraphicsAPI::eVulkan:
        {
            return Graphics::VulkanMapResource(&vulkanContextResources, handle);
            //break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
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
        case GraphicsAPI::eVulkan:
        {
            Graphics::VulkanUnmapResource(&vulkanContextResources, handle);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            runGame = false;
            break;
        }
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
            // TODO: support pressing escape in game
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
            switch (g_GlobalAppParams.m_graphicsAPI)
            {
                case GraphicsAPI::eVulkan:
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
                    Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
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
        g_GlobalAppParams.m_graphicsAPI = GraphicsAPI::eVulkan;
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

        switch (g_GlobalAppParams.m_graphicsAPI)
        {
            case GraphicsAPI::eVulkan:
            {
                vulkanContextResources = {};
                int result = InitVulkan(&vulkanContextResources, &g_platformWindowHandles, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight, g_SystemInfo.dwNumberOfProcessors / 2);
                if (result)
                {
                    Core::Utility::LogMsg("Platform", "Failed to init graphics backend!", Core::Utility::LogSeverity::eCritical);
                    return 1;
                }
                break;
            }

            default:
            {
                Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
                return 1;
            }
        }

        g_platformAPIFuncs = {};
        g_platformAPIFuncs.EnqueueWorkerThreadJob = EnqueueWorkerThreadJob;
        g_platformAPIFuncs.ReadEntireFile = ReadEntireFile;
        g_platformAPIFuncs.WriteEntireFile = WriteEntireFile;
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
        g_graphicsCommandStream.m_graphicsCommands = (GraphicsCommand*)_aligned_malloc_dbg(g_graphicsCommandStream.m_maxCommands * sizeof(GraphicsCommand), CACHE_LINE, __FILE__, __LINE__);

        #ifdef TINKER_PLATFORM_ENABLE_MULTITHREAD
        g_ThreadPool.Startup(g_SystemInfo.dwNumberOfProcessors / 2);
        #endif

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

                switch (g_GlobalAppParams.m_graphicsAPI)
                {
                    case GraphicsAPI::eVulkan:
                    {
                        if (g_windowResized)
                        {
                            VulkanDestroySwapChain(&vulkanContextResources);
                            VulkanCreateSwapChain(&vulkanContextResources);

                            g_GameCode.GameWindowResize(&g_platformAPIFuncs, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight);

                            g_windowResized = false;
                        }
                        shouldRenderFrame = vulkanContextResources.isSwapChainValid;

                        break;
                    }

                    default:
                    {
                        Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
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
                case GraphicsAPI::eVulkan:
                {
                    DestroyVulkan(&vulkanContextResources);
                    InitVulkan(&vulkanContextResources, &g_platformWindowHandles, g_GlobalAppParams.m_windowWidth, g_GlobalAppParams.m_windowHeight, g_SystemInfo.dwNumberOfProcessors / 2);
                    break;
                }

                default:
                {
                    Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
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
        case GraphicsAPI::eVulkan:
        {
            DestroyVulkan(&vulkanContextResources);
            break;
        }

        default:
        {
            Core::Utility::LogMsg("Platform", "Invalid/unsupported graphics API chosen!", Core::Utility::LogSeverity::eCritical);
            break;
        }
    }

    #if defined(MEM_TRACKING)
    _CrtDumpMemoryLeaks();
    #endif

    return 0;
}
