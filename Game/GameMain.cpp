#include "../Include/PlatformGameAPI.h"
#include "../Include/Core/Allocators.h"
#include "../Include/Core/Math/VectorTypes.h"
#include "../Include/Core/Containers/RingBuffer.h"
#include "GameGraphicsTypes.h"

#include <cstring>
#include <string.h>
#include <vector>

using namespace Tinker;
using namespace Core;
using namespace Math;

/*static void Test_Thread_Func()
{
    Platform::PrintDebugString("I am from a thread.\n");
}
*/

#define DEFAULT_QUAD_NUM_VERTICES 4
#define DEFAULT_QUAD_NUM_INDICES 6
DefaultGeometry<DEFAULT_QUAD_NUM_VERTICES, DEFAULT_QUAD_NUM_INDICES> defaultQuad = {
    TINKER_INVALID_HANDLE,
    TINKER_INVALID_HANDLE,
    TINKER_INVALID_HANDLE,
    TINKER_INVALID_HANDLE,
    v4f(-1.0f, -1.0f, 0.0f, 1.0f),
    v4f(1.0f, -1.0f, 0.0f, 1.0f),
    v4f(-1.0f, 1.0f, 0.0f, 1.0f),
    v4f(1.0f, 1.0f, 0.0f, 1.0f),
    //0, 1, 2, 2, 1, 3
    2, 1, 0, 3, 1, 2
};

static GameGraphicsData gameGraphicsData = {};

static std::vector<Platform::GraphicsCommand> graphicsCommands;

static bool isGameInitted = false;
static const bool isMultiplayer = false;
static bool connectedToServer = false;

uint32 currentWindowWidth, currentWindowHeight;

Memory::LinearAllocator shaderBytecodeAllocator;
const uint32 totalShaderBytecodeMaxSizeInBytes = 1024 * 10;

typedef struct input_state
{
    Platform::KeycodeState keyCodes[Platform::eMaxKeycodes];
} InputState;
static InputState currentInputState = {};
static InputState previousInputState = {};

typedef struct descriptor_instance_data
{
    alignas(16) m4f modelMatrix;
    alignas(16) m4f viewProj;
} DescriptorInstanceData;

static const v3f WORLD_UP = v3f(0, 0, 1);
typedef struct virtual_camera_data
{
    v3f m_eye;
    v3f m_ref;
} VirtualCamera;
static VirtualCamera g_gameCamera = {};
static float fovy = 0.785398f;
static const float nearPlane = 0.1f;
static const float farPlane = 1000.0f;
static m4f g_projMat = m4f(1.0f);

m4f CameraViewMatrix(const VirtualCamera* camera)
{
    v3f forward = camera->m_ref - camera->m_eye;
    Normalize(forward);

    v3f right = Cross(forward, WORLD_UP);
    Normalize(right);
    v3f up = Cross(right, forward);
    Normalize(up);

    m4f view;
    view[0] = v4f(right.x, forward.x, up.x, 0.0f);
    view[1] = v4f(right.y, forward.y, up.y, 0.0f);
    view[2] = v4f(right.z, forward.z, up.z, 0.0f);
    view[3] = v4f(-Dot(camera->m_eye, right), -Dot(camera->m_eye, forward), -Dot(camera->m_eye, up), 1.0f);
    return view;
}

m4f PerspectiveProjectionMatrix()
{
    m4f proj;

    const float aspect = (float)currentWindowWidth / currentWindowHeight;
    const float tanFov = tanf(fovy * 0.5f);

    proj[0][0] = 1.0f / (aspect * tanFov); proj[1][0] = 0.0f;                                             proj[2][0] = 0.0f;          proj[3][0] = 0.0f; // transform x to ndc
    proj[0][1] = 0.0f;                     proj[1][1] = 0.0f;                                             proj[2][1] = 1.0f / tanFov; proj[3][1] = 0.0f; // Z-up so y-coord gets normalized as depth
    proj[0][2] = 0.0f;                     proj[1][2] = (-nearPlane - farPlane) / (nearPlane - farPlane); proj[2][2] = 0.0f;          proj[3][2] = (2.0f * farPlane * nearPlane) / (nearPlane - farPlane); // Z-up, so y-coord means depth
    proj[0][3] = 0.0f;                     proj[1][3] = 1.0f;                                             proj[2][3] = 0.0f;          proj[3][3] = 0.0f; // Z-up, so eye-space y-coord is used for perspective divide

    return proj;
}

// TODO: remove me
#include <chrono>
void UpdateDescriptorState()
{
    // TODO: remove this, is just for testing
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    float scale = 1.0f; // sinf(time) * 0.5f + 0.5f;
    DescriptorInstanceData instanceData = {};
    instanceData.modelMatrix = m4f(scale);
    instanceData.modelMatrix[3][3] = 1.0f;
    instanceData.viewProj = g_projMat * CameraViewMatrix(&g_gameCamera);
    memcpy(gameGraphicsData.m_modelMatrixBufferMemPtr1, &instanceData, sizeof(instanceData));
}

uint32 LoadShader(const Platform::PlatformAPIFuncs* platformFuncs, const char* vertexShaderFileName, const char* fragmentShaderFileName, Platform::GraphicsPipelineParams* params)
{
    uint32 vertexShaderFileSize = platformFuncs->GetFileSize(vertexShaderFileName);
    uint32 fragmentShaderFileSize = platformFuncs->GetFileSize(fragmentShaderFileName);
    TINKER_ASSERT(vertexShaderFileSize > 0);
    TINKER_ASSERT(fragmentShaderFileSize > 0);

    uint8* vertexShaderBuffer = shaderBytecodeAllocator.Alloc(vertexShaderFileSize, 1);
    uint8* fragmentShaderBuffer = shaderBytecodeAllocator.Alloc(fragmentShaderFileSize, 1);
    void* vertexShaderCode = platformFuncs->ReadEntireFile(vertexShaderFileName, vertexShaderFileSize, vertexShaderBuffer);
    void* fragmentShaderCode = platformFuncs->ReadEntireFile(fragmentShaderFileName, fragmentShaderFileSize, fragmentShaderBuffer);

    return platformFuncs->CreateGraphicsPipeline(vertexShaderCode, vertexShaderFileSize, fragmentShaderCode, fragmentShaderFileSize, params->blendState, params->depthState, params->viewportWidth, params->viewportHeight, params->renderPassHandle, params->descriptorHandle);
}

void LoadAllShaders(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    shaderBytecodeAllocator.Free();
    shaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);

    Platform::DescriptorLayout mainDrawDescriptorLayout = {};
    Platform::InitDescLayout(&mainDrawDescriptorLayout);
    Platform::GraphicsPipelineParams params;
    params.blendState = Platform::eBlendStateAlphaBlend;
    params.depthState = Platform::eDepthStateTestOnWriteOn;
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.renderPassHandle = gameGraphicsData.m_mainRenderPassHandle;
    params.descriptorHandle = gameGraphicsData.m_modelMatrixDescHandle1;
    gameGraphicsData.m_shaderHandle = LoadShader(platformFuncs, "..\\Shaders\\spv\\basic_glsl_vert.spv", "..\\Shaders\\spv\\basic_glsl_frag.spv", &params);

    params.blendState = Platform::eBlendStateAlphaBlend;
    params.depthState = Platform::eDepthStateOff;
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.renderPassHandle = TINKER_INVALID_HANDLE;
    params.descriptorHandle = gameGraphicsData.m_swapChainBlitDescHandle;
    gameGraphicsData.m_blitShaderHandle = LoadShader(platformFuncs, "..\\Shaders\\spv\\blit_glsl_vert.spv", "..\\Shaders\\spv\\blit_glsl_frag.spv", &params);
}

void DestroyShaders(const Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_shaderHandle);
    platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_blitShaderHandle);
    gameGraphicsData.m_shaderHandle = TINKER_INVALID_HANDLE;
    gameGraphicsData.m_blitShaderHandle = TINKER_INVALID_HANDLE;
}

void DestroyDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyDescriptor(gameGraphicsData.m_swapChainBlitDescHandle);
    gameGraphicsData.m_swapChainBlitDescHandle = TINKER_INVALID_HANDLE;

    platformFuncs->DestroyDescriptor(gameGraphicsData.m_modelMatrixDescHandle1);
    gameGraphicsData.m_modelMatrixDescHandle1 = TINKER_INVALID_HANDLE;

    platformFuncs->DestroyAllDescriptors(); // TODO: this is not a good API and should be per-pool or something
}

void CreateAllDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    // Swap chain blit
    Platform::DescriptorLayout blitDescriptorLayout = {};
    Platform::InitDescLayout(&blitDescriptorLayout);
    blitDescriptorLayout.descriptorTypes[0][0].type = Platform::eDescriptorTypeSampledImage;
    blitDescriptorLayout.descriptorTypes[0][0].amount = 1;
    gameGraphicsData.m_swapChainBlitDescHandle = platformFuncs->CreateDescriptor(&blitDescriptorLayout);

    Platform::DescriptorSetDataHandles blitHandles = {};
    blitHandles.handles[0] = gameGraphicsData.m_imageViewHandle;
    platformFuncs->WriteDescriptor(&blitDescriptorLayout, gameGraphicsData.m_swapChainBlitDescHandle, &blitHandles);

    // Model matrix
    Platform::BufferData bufferData = platformFuncs->CreateBuffer(sizeof(DescriptorInstanceData), Platform::eBufferUsageUniform);
    gameGraphicsData.m_modelMatrixBufferHandle1 = bufferData.handle;
    gameGraphicsData.m_modelMatrixBufferMemPtr1 = bufferData.memory;

    Platform::DescriptorLayout instanceDataDescriptorLayout = {};
    Platform::InitDescLayout(&instanceDataDescriptorLayout);
    instanceDataDescriptorLayout.descriptorTypes[0][0].type = Platform::eDescriptorTypeBuffer;
    instanceDataDescriptorLayout.descriptorTypes[0][0].amount = 1;
    gameGraphicsData.m_modelMatrixDescHandle1 = platformFuncs->CreateDescriptor(&instanceDataDescriptorLayout);

    Platform::DescriptorSetDataHandles modeMatrixHandles = {};
    modeMatrixHandles.handles[0] = gameGraphicsData.m_modelMatrixBufferHandle1;
    platformFuncs->WriteDescriptor(&instanceDataDescriptorLayout, gameGraphicsData.m_modelMatrixDescHandle1, &modeMatrixHandles);
}

void RecreateShaders(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    DestroyShaders(platformFuncs);
    
    DestroyDescriptors(platformFuncs);

    CreateAllDescriptors(platformFuncs);
    LoadAllShaders(platformFuncs, windowWidth, windowHeight);
}

void CreateGameRenderingResources(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    gameGraphicsData.m_imageHandle = platformFuncs->CreateImageResource(windowWidth, windowHeight);
    gameGraphicsData.m_imageViewHandle = platformFuncs->CreateImageViewResource(gameGraphicsData.m_imageHandle);
    gameGraphicsData.m_mainRenderPassHandle = platformFuncs->CreateRenderPass(Platform::eImageLayoutUndefined, Platform::eImageLayoutShaderRead);
    gameGraphicsData.m_framebufferHandle = platformFuncs->CreateFramebuffer(&gameGraphicsData.m_imageViewHandle, 1, windowWidth, windowHeight, gameGraphicsData.m_mainRenderPassHandle);
}

void ProcessInputState(const Platform::InputStateDeltas* inputStateDeltas, const Platform::PlatformAPIFuncs* platformFuncs)
{
    previousInputState = currentInputState;

    for (uint32 uiKeycode = 0; uiKeycode < Platform::eMaxKeycodes; ++uiKeycode)
    {
        if (inputStateDeltas->keyCodes[uiKeycode].numStateChanges > 0)
        {
            currentInputState.keyCodes[uiKeycode].isDown = inputStateDeltas->keyCodes[uiKeycode].isDown;
            currentInputState.keyCodes[uiKeycode].numStateChanges += inputStateDeltas->keyCodes[uiKeycode].numStateChanges;
            // TODO: reset number of state changes at some point, every second or every several frames or something
        }
    }

    for (uint32 uiKeycode = 0; uiKeycode < Platform::eMaxKeycodes; ++uiKeycode)
    {
        switch (uiKeycode)
        {
            case Platform::eKeyF10:
            {
                // Handle the initial downpress once and nothing more
                if (currentInputState.keyCodes[uiKeycode].isDown && !previousInputState.keyCodes[uiKeycode].isDown)
                {
                    Platform::PrintDebugString("Attempting to hotload shaders...\n");

                    // Recompile shaders via script
                    const char* shaderCompileCommand = "..\\Scripts\\compile_shaders_glsl2spv.bat";
                    if (platformFuncs->SystemCommand(shaderCompileCommand) != 0)
                    {
                        Platform::PrintDebugString("Failed to create shader compile process! Shaders will not be compiled.\n");
                    }
                    else
                    {
                        // Recreate gpu resources
                        RecreateShaders(platformFuncs, currentWindowWidth, currentWindowHeight);
                        Platform::PrintDebugString("...Done.\n");
                    }
                }

                // Handle as long as the key is down
                if (currentInputState.keyCodes[uiKeycode].isDown)
                {
                }
                
                break;
            }

            default:
            {
                break;
            }
        }
    }
}

extern "C"
GAME_UPDATE(GameUpdate)
{
    graphicsCommands.clear();

    if (!isGameInitted)
    {
        //g_gameCamera.m_eye = v3f(3.0f, 3.0f, 3.0f);
        g_gameCamera.m_ref = v3f(0.0f, 0.0f, 0.0f);
        currentWindowWidth = windowWidth;
        currentWindowHeight = windowHeight;
        g_projMat = PerspectiveProjectionMatrix();

        // Init network connection if multiplayer
        if (isMultiplayer)
        {
            int result = platformFuncs->InitNetworkConnection();
            if (result != 0)
            {
                connectedToServer = false;
                return 1;
            }
            else
            {
                connectedToServer = true;
            }
        }

        graphicsCommands.reserve(graphicsCommandStream->m_maxCommands);

        // Default geometry
        defaultQuad.m_vertexBufferHandle = platformFuncs->CreateBuffer(sizeof(defaultQuad.m_points), Platform::eBufferUsageVertex).handle;
        defaultQuad.m_indexBufferHandle = platformFuncs->CreateBuffer(sizeof(defaultQuad.m_indices), Platform::eBufferUsageIndex).handle;
        Platform::BufferData stagingBufferData = platformFuncs->CreateBuffer(sizeof(defaultQuad.m_points), Platform::eBufferUsageStaging);
        defaultQuad.m_stagingBufferHandle_vert = stagingBufferData.handle;
        memcpy(stagingBufferData.memory, defaultQuad.m_points, sizeof(defaultQuad.m_points));
        stagingBufferData = platformFuncs->CreateBuffer(sizeof(defaultQuad.m_indices), Platform::eBufferUsageStaging);
        defaultQuad.m_stagingBufferHandle_idx = stagingBufferData.handle;
        memcpy(stagingBufferData.memory, defaultQuad.m_indices, sizeof(defaultQuad.m_points));

        Platform::GraphicsCommand command;
        command.m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;
        command.m_sizeInBytes = sizeof(defaultQuad.m_points);
        command.m_dstBufferHandle = defaultQuad.m_vertexBufferHandle;
        command.m_srcBufferHandle = defaultQuad.m_stagingBufferHandle_vert;
        graphicsCommands.push_back(command);
        command.m_sizeInBytes = sizeof(defaultQuad.m_indices);
        command.m_dstBufferHandle = defaultQuad.m_indexBufferHandle;
        command.m_srcBufferHandle = defaultQuad.m_stagingBufferHandle_idx;
        graphicsCommands.push_back(command);

        // Game graphics
        uint32 numVertBytes = sizeof(v4f) * 4; // aligned memory size
        uint32 numIdxBytes = sizeof(uint32) * 16; // aligned memory size
        uint32 vertexBufferHandle = platformFuncs->CreateBuffer(numVertBytes, Platform::eBufferUsageVertex).handle;
        Platform::BufferData data = platformFuncs->CreateBuffer(numVertBytes, Platform::eBufferUsageStaging);
        uint32 stagingBufferHandle = data.handle;
        void* stagingBufferMemPtr = data.memory;
        uint32 indexBufferHandle = platformFuncs->CreateBuffer(numIdxBytes, Platform::eBufferUsageIndex).handle;
        data = platformFuncs->CreateBuffer(numIdxBytes, Platform::eBufferUsageStaging);
        uint32 stagingBufferHandle3 = data.handle;
        void* stagingBufferMemPtr3 = data.memory;

        uint32 vertexBufferHandle2 = platformFuncs->CreateBuffer(numVertBytes, Platform::eBufferUsageVertex).handle;
        data = platformFuncs->CreateBuffer(numVertBytes, Platform::eBufferUsageStaging);
        uint32 stagingBufferHandle2 = data.handle;
        void* stagingBufferMemPtr2 = data.memory;
        uint32 indexBufferHandle2 = platformFuncs->CreateBuffer(numIdxBytes, Platform::eBufferUsageIndex).handle;
        data = platformFuncs->CreateBuffer(numIdxBytes, Platform::eBufferUsageStaging);
        uint32 stagingBufferHandle4 = data.handle;
        void* stagingBufferMemPtr4 = data.memory;

        gameGraphicsData.m_vertexBufferHandle = vertexBufferHandle;
        gameGraphicsData.m_stagingBufferHandle = stagingBufferHandle;
        gameGraphicsData.m_stagingBufferMemPtr = stagingBufferMemPtr;
        gameGraphicsData.m_indexBufferHandle = indexBufferHandle;
        gameGraphicsData.m_stagingBufferHandle3 = stagingBufferHandle3;
        gameGraphicsData.m_stagingBufferMemPtr3 = stagingBufferMemPtr3;

        gameGraphicsData.m_vertexBufferHandle2 = vertexBufferHandle2;
        gameGraphicsData.m_stagingBufferHandle2 = stagingBufferHandle2;
        gameGraphicsData.m_stagingBufferMemPtr2 = stagingBufferMemPtr2;
        gameGraphicsData.m_indexBufferHandle2 = indexBufferHandle2;
        gameGraphicsData.m_stagingBufferHandle4 = stagingBufferHandle4;
        gameGraphicsData.m_stagingBufferMemPtr4 = stagingBufferMemPtr4;

        CreateGameRenderingResources(platformFuncs, windowWidth, windowHeight);

        CreateAllDescriptors(platformFuncs);
        LoadAllShaders(platformFuncs, windowWidth, windowHeight);

        isGameInitted = true;
    }

    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    g_gameCamera.m_eye = v3f(cosf(time) * 3.0f, sinf(time) * 3.0f, 1.0f);

    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;

    ProcessInputState(inputStateDeltas, platformFuncs);
    UpdateDescriptorState();

    uint32 numVertBytes = sizeof(v4f) * 3;
    uint32 numIdxBytes = sizeof(uint32) * 3;
    v4f positions[] = { v4f(0.0f, -0.5f, sinf(time) * 0.2f, 1.0f), v4f(0.5f, 0.5f, sinf(time) * 0.2f, 1.0f), v4f(-0.5f, 0.5f, sinf(time) * 0.2f, 1.0f) };
    uint32 indices[] = { 0, 2, 1 };
    memcpy(gameGraphicsData.m_stagingBufferMemPtr, positions, numVertBytes);
    memcpy(gameGraphicsData.m_stagingBufferMemPtr3, indices, numIdxBytes);

    v4f positions2[] = { v4f(0.45f, -0.5f, 0.0f, 1.0f), v4f(0.75f, 0.25f, 0.0f, 1.0f), v4f(0.15f, 0.25f, 0.0f, 1.0f) };
    uint32 indices2[] = { 0, 2, 1 };
    memcpy(gameGraphicsData.m_stagingBufferMemPtr2, positions2, numVertBytes);
    memcpy(gameGraphicsData.m_stagingBufferMemPtr4, indices2, numIdxBytes);

    //Platform::PrintDebugString("Joe\n");

    // Test a thread job
    /*Platform::WorkerJob* jobs[32] = {};
    for (uint32 i = 0; i < 32; ++i)
    {
        jobs[i] = Platform::CreateNewThreadJob([=]()
                {
                    // TODO: custom string library...
                    char* string = "I am from a thread";
                    char dst[50] = "";
                    strcpy_s(dst, string);
                    _itoa_s(i, dst + strlen(string), 10, 10);
                    if (i < 10) dst[strlen(string) + 1] = ' ';
                    dst[strlen(string) + 2] = '\n';
                    dst[strlen(string) + 3] = '\0';
                    Platform::PrintDebugString(dst);
                });
        platformFuncs->EnqueueWorkerThreadJob(jobs[i]);
    }
    for (uint32 i = 0; i < 32; ++i)
    {
        platformFuncs->WaitOnThreadJob(jobs[i]);
    }*/
    
    // Issue graphics commands
    Platform::GraphicsCommand command;

    command.m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;
    command.m_sizeInBytes = numVertBytes;
    command.m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle;
    command.m_dstBufferHandle = gameGraphicsData.m_vertexBufferHandle;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;
    command.m_sizeInBytes = numVertBytes;
    command.m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle2;
    command.m_dstBufferHandle = gameGraphicsData.m_vertexBufferHandle2;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;
    command.m_sizeInBytes = numIdxBytes;
    command.m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle3;
    command.m_dstBufferHandle = gameGraphicsData.m_indexBufferHandle;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Platform::eGraphicsCmdMemTransfer;
    command.m_sizeInBytes = numIdxBytes;
    command.m_srcBufferHandle = gameGraphicsData.m_stagingBufferHandle4;
    command.m_dstBufferHandle = gameGraphicsData.m_indexBufferHandle2;
    graphicsCommands.push_back(command);

    /*command.m_commandType = (uint32)Platform::eGraphicsCmdImageCopy;
    command.m_srcImgHandle = gameGraphicsData.m_imageHandle;
    command.m_dstImgHandle = TINKER_INVALID_HANDLE;
    command.m_width = windowWidth;
    command.m_height = windowHeight;
    graphicsCommands.push_back(command);*/

    command.m_commandType = (uint32)Platform::eGraphicsCmdRenderPassBegin;
    command.m_renderPassHandle = gameGraphicsData.m_mainRenderPassHandle;
    command.m_framebufferHandle = gameGraphicsData.m_framebufferHandle;
    command.m_renderWidth = windowWidth;
    command.m_renderHeight = windowHeight;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Platform::eGraphicsCmdDrawCall;
    command.m_numIndices = 3;
    command.m_numUVs = 0;
    command.m_numVertices = 3;
    command.m_indexBufferHandle = gameGraphicsData.m_indexBufferHandle;
    command.m_vertexBufferHandle = gameGraphicsData.m_vertexBufferHandle;
    command.m_uvBufferHandle = TINKER_INVALID_HANDLE;
    command.m_shaderHandle = gameGraphicsData.m_shaderHandle;
    Platform::InitDescSetDataHandles(command.m_descriptors);
    command.m_descriptors[0].handles[0] = gameGraphicsData.m_modelMatrixDescHandle1;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Platform::eGraphicsCmdDrawCall;
    command.m_numIndices = 3;
    command.m_numUVs = 0;
    command.m_numVertices = 3;
    command.m_indexBufferHandle = gameGraphicsData.m_indexBufferHandle2;
    command.m_vertexBufferHandle = gameGraphicsData.m_vertexBufferHandle2;
    command.m_uvBufferHandle = TINKER_INVALID_HANDLE;
    command.m_shaderHandle = gameGraphicsData.m_shaderHandle;
    Platform::InitDescSetDataHandles(command.m_descriptors);
    command.m_descriptors[0].handles[0] = gameGraphicsData.m_modelMatrixDescHandle1;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Platform::eGraphicsCmdRenderPassEnd;
    graphicsCommands.push_back(command);

    /*command.m_commandType = (uint32)Platform::eGraphicsCmdImageCopy;
    command.m_srcImgHandle = gameGraphicsData.m_imageHandle;
    command.m_dstImgHandle = TINKER_INVALID_HANDLE;
    command.m_width = windowWidth;
    command.m_height = windowHeight;
    graphicsCommands.push_back(command);*/

    // Blit to screen
    command.m_commandType = (uint32)Platform::eGraphicsCmdRenderPassBegin;
    command.m_renderPassHandle = TINKER_INVALID_HANDLE;
    command.m_framebufferHandle = TINKER_INVALID_HANDLE;
    command.m_renderWidth = 0;
    command.m_renderHeight = 0;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Platform::eGraphicsCmdDrawCall;
    command.m_numIndices = DEFAULT_QUAD_NUM_INDICES;
    command.m_numUVs = 0;
    command.m_numVertices = DEFAULT_QUAD_NUM_VERTICES;
    command.m_vertexBufferHandle = defaultQuad.m_vertexBufferHandle;
    command.m_indexBufferHandle = defaultQuad.m_indexBufferHandle;
    command.m_uvBufferHandle = TINKER_INVALID_HANDLE;
    command.m_shaderHandle = gameGraphicsData.m_blitShaderHandle;
    Platform::InitDescSetDataHandles(command.m_descriptors);
    command.m_descriptors[0].handles[0] = gameGraphicsData.m_swapChainBlitDescHandle;
    graphicsCommands.push_back(command);

    command.m_commandType = (uint32)Platform::eGraphicsCmdRenderPassEnd;
    graphicsCommands.push_back(command);

    graphicsCommandStream->m_numCommands = (uint32)graphicsCommands.size();
    Platform::GraphicsCommand* graphicsCmdBase = graphicsCommandStream->m_graphicsCommands;
    uint32 graphicsCmdSizeInBytes = (uint32)graphicsCommands.size() * sizeof(Platform::GraphicsCommand);
    memcpy(graphicsCmdBase, graphicsCommands.data(), graphicsCmdSizeInBytes);
    graphicsCmdBase += graphicsCmdSizeInBytes;

    if (isGameInitted && isMultiplayer && connectedToServer)
    {
        int result = platformFuncs->SendMessageToServer();
        if (result != 0)
        {
            return 1;
        }
        else
        {
            // Sent message successfully
        }
    }

    return 0;
}

void DestroyWindowResizeDependentResources(const Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyRenderPass(gameGraphicsData.m_mainRenderPassHandle);
    platformFuncs->DestroyFramebuffer(gameGraphicsData.m_framebufferHandle);
    platformFuncs->DestroyImageResource(gameGraphicsData.m_imageHandle);
    platformFuncs->DestroyImageViewResource(gameGraphicsData.m_imageViewHandle);

    // Instance uniform data
    platformFuncs->DestroyBuffer(gameGraphicsData.m_modelMatrixBufferHandle1, Platform::eBufferUsageUniform);

    DestroyShaders(platformFuncs);
    DestroyDescriptors(platformFuncs);
}

extern "C"
GAME_WINDOW_RESIZE(GameWindowResize)
{
    currentWindowWidth = newWindowWidth;
    currentWindowHeight = newWindowHeight;
    DestroyWindowResizeDependentResources(platformFuncs);

    // Gameplay stuff
    g_projMat = PerspectiveProjectionMatrix();

    CreateGameRenderingResources(platformFuncs, newWindowWidth, newWindowHeight);
    CreateAllDescriptors(platformFuncs);
    LoadAllShaders(platformFuncs, newWindowWidth, newWindowHeight);
}

extern "C"
GAME_DESTROY(GameDestroy)
{
    if (isGameInitted)
    {
        DestroyWindowResizeDependentResources(platformFuncs);

        // Default geometry
        platformFuncs->DestroyBuffer(defaultQuad.m_vertexBufferHandle, Platform::eBufferUsageVertex);
        platformFuncs->DestroyBuffer(defaultQuad.m_indexBufferHandle, Platform::eBufferUsageIndex);
        platformFuncs->DestroyBuffer(defaultQuad.m_stagingBufferHandle_vert, Platform::eBufferUsageStaging);
        platformFuncs->DestroyBuffer(defaultQuad.m_stagingBufferHandle_idx, Platform::eBufferUsageStaging);

        // Game graphics
        platformFuncs->DestroyBuffer(gameGraphicsData.m_vertexBufferHandle, Platform::eBufferUsageVertex);
        platformFuncs->DestroyBuffer(gameGraphicsData.m_vertexBufferHandle2, Platform::eBufferUsageVertex);
        platformFuncs->DestroyBuffer(gameGraphicsData.m_stagingBufferHandle, Platform::eBufferUsageStaging);
        platformFuncs->DestroyBuffer(gameGraphicsData.m_stagingBufferHandle2, Platform::eBufferUsageStaging);
        platformFuncs->DestroyBuffer(gameGraphicsData.m_indexBufferHandle, Platform::eBufferUsageIndex);
        platformFuncs->DestroyBuffer(gameGraphicsData.m_indexBufferHandle2, Platform::eBufferUsageIndex);
        platformFuncs->DestroyBuffer(gameGraphicsData.m_stagingBufferHandle3, Platform::eBufferUsageStaging);
        platformFuncs->DestroyBuffer(gameGraphicsData.m_stagingBufferHandle4, Platform::eBufferUsageStaging);

        if (isMultiplayer && connectedToServer)
        {
            platformFuncs->EndNetworkConnection();
        }
    }
}
