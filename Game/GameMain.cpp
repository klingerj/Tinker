#include "PlatformGameAPI.h"
#include "Core/Allocators.h"
#include "Core/Math/VectorTypes.h"
#include "Core/FileIO/FileLoading.h"
#include "Core/Utility/ScopedTimer.h"
#include "GraphicsTypes.h"
#include "ShaderLoading.h"
#include "RenderPass.h"
#include "AssetManager.h"
#include "Camera.h"
#include "Raytracing.h"
#include "View.h"
#include "InputManager.h"

#ifdef _SCRIPTS_DIR
#define SCRIPTS_PATH STRINGIFY(_SCRIPTS_DIR)
#else
//#define SCRIPTS_PATH "..\\Scripts\\"
#endif

#include <string.h>

using namespace Tk;
using namespace Core;
using namespace Math;

static bool isGameInitted = false;
static const bool isMultiplayer = false;
static bool connectedToServer = false;
uint32 currentWindowWidth = 0, currentWindowHeight = 0;

static GameGraphicsData gameGraphicsData = {};
static GameRenderPass gameRenderPasses[eRenderPass_Max] = {};

static Camera g_gameCamera = {};
static const float cameraPanSensitivity = 0.1f;
static const float cameraRotSensitivityHorz = 0.001f;
static const float cameraRotSensitivityVert = 0.002f;
INPUT_CALLBACK(GameCameraPanForwardCallback)
{
    PanCameraAlongForward(&g_gameCamera, cameraPanSensitivity * param);
}
INPUT_CALLBACK(GameCameraPanBackwardCallback)
{
    PanCameraAlongForward(&g_gameCamera, -cameraPanSensitivity * param);
}
INPUT_CALLBACK(GameCameraPanRightCallback)
{
    PanCameraAlongRight(&g_gameCamera, cameraPanSensitivity * param);
}
INPUT_CALLBACK(GameCameraPanLeftCallback)
{
    PanCameraAlongRight(&g_gameCamera, -cameraPanSensitivity * param);
}
INPUT_CALLBACK(GameCameraRotateHorizontalCallback)
{
    RotateCameraAboutUp(&g_gameCamera, cameraRotSensitivityHorz * -(int32)param);
}
INPUT_CALLBACK(GameCameraRotateVerticalCallback)
{
    RotateCameraAboutRight(&g_gameCamera, cameraRotSensitivityVert * -(int32)param);
}

#define MAX_INSTANCES_PER_VIEW 128
static View MainView;

INPUT_CALLBACK(RaytraceTestCallback)
{
    Platform::PrintDebugString("Running raytrace test...\n");
    RaytraceTest(platformFuncs);
    Platform::PrintDebugString("...Done.\n");
}

void LoadAllShaders(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    ResetShaderBytecodeAllocator();

    Platform::DescriptorLayout mainDrawDescriptorLayout = {};
    mainDrawDescriptorLayout.InitInvalid();
    Platform::GraphicsPipelineParams params;
    params.blendState = Platform::BlendState::eInvalid; // No color attachment
    params.depthState = Platform::DepthState::eTestOnWriteOn; // Write to depth
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.framebufferHandle = gameGraphicsData.m_framebufferHandles[eRenderPass_ZPrePass];

    const uint32 numDescriptorsPerShader = 3;
    Platform::DescriptorHandle descHandles[numDescriptorsPerShader] = { gameGraphicsData.m_DescData_Global, gameGraphicsData.m_DescData_Instance, g_AssetManager.GetMeshGraphicsDataByID(0)->m_descriptor }; // TODO: bad
    params.descriptorHandles = descHandles;
    params.numDescriptorHandles = numDescriptorsPerShader;
    gameGraphicsData.m_shaderHandles[eRenderPass_ZPrePass] = LoadShader(platformFuncs, SHADERS_SPV_PATH "basic_vert_glsl.spv", nullptr, &params);

    params.framebufferHandle = gameGraphicsData.m_framebufferHandles[eRenderPass_MainView];
    params.blendState = Platform::BlendState::eAlphaBlend; // Default alpha blending for now
    params.depthState = Platform::DepthState::eTestOnWriteOff; // Read depth buffer, don't write to it
    gameGraphicsData.m_shaderHandles[eRenderPass_MainView] = LoadShader(platformFuncs, SHADERS_SPV_PATH "basic_vert_glsl.spv", SHADERS_SPV_PATH "basic_frag_glsl.spv", &params);

    gameRenderPasses[eRenderPass_ZPrePass].shader = gameGraphicsData.m_shaderHandles[eRenderPass_ZPrePass];
    gameRenderPasses[eRenderPass_MainView].shader = gameGraphicsData.m_shaderHandles[eRenderPass_MainView];

    params.blendState = Platform::BlendState::eAlphaBlend;
    params.depthState = Platform::DepthState::eOff;
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.framebufferHandle = Platform::DefaultFramebufferHandle_Invalid;
    descHandles[0] = gameGraphicsData.m_swapChainBlitDescHandle;
    descHandles[1] = g_AssetManager.GetMeshGraphicsDataByID(0)->m_descriptor;
    descHandles[2] = Platform::DefaultDescHandle_Invalid;
    params.numDescriptorHandles = 2;
    gameGraphicsData.m_blitShaderHandle = LoadShader(platformFuncs, SHADERS_SPV_PATH "blit_vert_glsl.spv", SHADERS_SPV_PATH "blit_frag_glsl.spv", &params);
}

void DestroyShaders(const Platform::PlatformAPIFuncs* platformFuncs)
{
    for (uint32 uiPass = 0; uiPass < eRenderPass_Max; ++uiPass)
    {
        platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_shaderHandles[uiPass]);
        gameGraphicsData.m_shaderHandles[uiPass] = Platform::DefaultShaderHandle_Invalid;
    }

    platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_blitShaderHandle);
    gameGraphicsData.m_blitShaderHandle = Platform::DefaultShaderHandle_Invalid;
}

void DestroyDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyDescriptor(gameGraphicsData.m_swapChainBlitDescHandle);
    gameGraphicsData.m_swapChainBlitDescHandle = Platform::DefaultDescHandle_Invalid;

    platformFuncs->DestroyDescriptor(gameGraphicsData.m_DescData_Instance);
    gameGraphicsData.m_DescData_Instance = Platform::DefaultDescHandle_Invalid;
    platformFuncs->DestroyResource(gameGraphicsData.m_DescDataBufferHandle_Instance);
    gameGraphicsData.m_DescDataBufferHandle_Instance = Platform::DefaultResHandle_Invalid;

    platformFuncs->DestroyDescriptor(gameGraphicsData.m_DescData_Global);
    gameGraphicsData.m_DescData_Global = Platform::DefaultDescHandle_Invalid;
    platformFuncs->DestroyResource(gameGraphicsData.m_DescDataBufferHandle_Global);
    gameGraphicsData.m_DescDataBufferHandle_Global = Platform::DefaultResHandle_Invalid;

    platformFuncs->DestroyAllDescriptors(); // destroys descriptor pool
}

void WriteSwapChainBlitResources(const Platform::PlatformAPIFuncs* platformFuncs)
{
    Platform::DescriptorLayout blitDescriptorLayout = {};
    blitDescriptorLayout.InitInvalid();
    blitDescriptorLayout.descriptorLayoutParams[0][0].type = Platform::DescriptorType::eSampledImage;
    blitDescriptorLayout.descriptorLayoutParams[0][0].amount = 1;

    Platform::DescriptorSetDataHandles blitHandles = {};
    blitHandles.InitInvalid();
    blitHandles.handles[0] = gameGraphicsData.m_rtColorHandle;
    platformFuncs->WriteDescriptor(&blitDescriptorLayout, &gameGraphicsData.m_swapChainBlitDescHandle, &blitHandles);
}

void CreateAllDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    // Swap chain blit
    Platform::DescriptorLayout blitDescriptorLayout = {};
    blitDescriptorLayout.InitInvalid();
    blitDescriptorLayout.descriptorLayoutParams[0][0].type = Platform::DescriptorType::eSampledImage;
    blitDescriptorLayout.descriptorLayoutParams[0][0].amount = 1;
    gameGraphicsData.m_swapChainBlitDescHandle = platformFuncs->CreateDescriptor(&blitDescriptorLayout);

    WriteSwapChainBlitResources(platformFuncs);

    // Descriptor data
    Platform::ResourceDesc desc;
    desc.resourceType = Platform::ResourceType::eBuffer1D;
    desc.dims = v3ui(sizeof(DescriptorData_Instance) * MAX_INSTANCES_PER_VIEW, 0, 0);
    desc.bufferUsage = Platform::BufferUsage::eUniform;
    gameGraphicsData.m_DescDataBufferHandle_Instance = platformFuncs->CreateResource(desc);
    desc.dims = v3ui(sizeof(DescriptorData_Global), 0, 0);
    gameGraphicsData.m_DescDataBufferHandle_Global = platformFuncs->CreateResource(desc);

    Platform::DescriptorLayout descriptorLayout = {};
    descriptorLayout.InitInvalid();
    descriptorLayout.descriptorLayoutParams[0][0].type = Platform::DescriptorType::eBuffer;
    descriptorLayout.descriptorLayoutParams[0][0].amount = 1;
    gameGraphicsData.m_DescData_Global = platformFuncs->CreateDescriptor(&descriptorLayout);

    Platform::DescriptorHandle descHandles[MAX_DESCRIPTORS_PER_SET] = { gameGraphicsData.m_DescData_Global, Platform::DefaultDescHandle_Invalid, Platform::DefaultDescHandle_Invalid };

    Platform::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    descDataHandles[0].InitInvalid();
    descDataHandles[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Global;
    descDataHandles[1].InitInvalid();
    platformFuncs->WriteDescriptor(&descriptorLayout, &descHandles[0], &descDataHandles[0]);

    descriptorLayout.InitInvalid();
    descriptorLayout.descriptorLayoutParams[1][0].type = Platform::DescriptorType::eBuffer;
    descriptorLayout.descriptorLayoutParams[1][0].amount = 1;
    gameGraphicsData.m_DescData_Instance = platformFuncs->CreateDescriptor(&descriptorLayout);

    descHandles[0] = Platform::DefaultDescHandle_Invalid;
    descHandles[1] = gameGraphicsData.m_DescData_Instance;

    descDataHandles[0].InitInvalid();
    descDataHandles[1].InitInvalid();
    descDataHandles[1].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Instance;
    platformFuncs->WriteDescriptor(&descriptorLayout, &descHandles[0], &descDataHandles[0]);
}

void RecreateShaders(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    DestroyShaders(platformFuncs);
    LoadAllShaders(platformFuncs, windowWidth, windowHeight);
}

INPUT_CALLBACK(ShaderHotloadCallback)
{
    Platform::PrintDebugString("Attempting to hotload shaders...\n");

    // Recompile shaders via script
    const char* shaderCompileCommand = SCRIPTS_PATH "build_compile_shaders_glsl2spv.bat";
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

void CreateGameRenderingResources(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    Platform::ResourceDesc desc;
    desc.resourceType = Platform::ResourceType::eImage2D;
    desc.dims = v3ui(windowWidth, windowHeight, 1);
    desc.imageFormat = Platform::ImageFormat::RGBA8_SRGB;
    gameGraphicsData.m_rtColorHandle = platformFuncs->CreateResource(desc);
    desc.imageFormat = Platform::ImageFormat::Depth_32F;
    gameGraphicsData.m_rtDepthHandle = platformFuncs->CreateResource(desc);

    // Depth-only pass
    gameGraphicsData.m_framebufferHandles[eRenderPass_ZPrePass] = platformFuncs->CreateFramebuffer(nullptr, 0, gameGraphicsData.m_rtDepthHandle, Platform::ImageLayout::eUndefined, windowWidth, windowHeight);

    // Color and depth
    gameGraphicsData.m_framebufferHandles[eRenderPass_MainView] = platformFuncs->CreateFramebuffer(&gameGraphicsData.m_rtColorHandle, 1, gameGraphicsData.m_rtDepthHandle, Platform::ImageLayout::eShaderRead, windowWidth, windowHeight);

    gameRenderPasses[eRenderPass_ZPrePass] = {};
    gameRenderPasses[eRenderPass_ZPrePass].framebuffer = gameGraphicsData.m_framebufferHandles[eRenderPass_ZPrePass];
    gameRenderPasses[eRenderPass_ZPrePass].renderWidth = windowWidth;
    gameRenderPasses[eRenderPass_ZPrePass].renderHeight = windowHeight;
    gameRenderPasses[eRenderPass_ZPrePass].debugLabel = "Z Prepass";

    gameRenderPasses[eRenderPass_MainView] = {};
    gameRenderPasses[eRenderPass_MainView].framebuffer = gameGraphicsData.m_framebufferHandles[eRenderPass_MainView];
    gameRenderPasses[eRenderPass_MainView].renderWidth = windowWidth;
    gameRenderPasses[eRenderPass_MainView].renderHeight = windowHeight;
    gameRenderPasses[eRenderPass_MainView].debugLabel = "Main Render View";
}

uint32 GameInit(const Tk::Platform::PlatformAPIFuncs* platformFuncs, Tk::Platform::GraphicsCommandStream* graphicsCommandStream, uint32 windowWidth, uint32 windowHeight)
{
    TIMED_SCOPED_BLOCK("Game Init");

    // Camera controls
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eW, GameCameraPanForwardCallback);
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eA, GameCameraPanLeftCallback);
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eS, GameCameraPanBackwardCallback);
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eD, GameCameraPanRightCallback);
    g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eW, GameCameraPanForwardCallback);
    g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eA, GameCameraPanLeftCallback);
    g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eS, GameCameraPanBackwardCallback);
    g_InputManager.BindKeycodeCallback_KeyDownRepeat(Platform::Keycode::eD, GameCameraPanRightCallback);
    g_InputManager.BindMousecodeCallback(Platform::Mousecode::eMouseMoveHorizontal, GameCameraRotateHorizontalCallback);
    g_InputManager.BindMousecodeCallback(Platform::Mousecode::eMouseMoveVertical, GameCameraRotateVerticalCallback);

    // Hotkeys
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eF9, RaytraceTestCallback);
    g_InputManager.BindKeycodeCallback_KeyDown(Platform::Keycode::eF10, ShaderHotloadCallback);

    g_gameCamera.m_ref = v3f(0.0f, 0.0f, 0.0f);
    g_gameCamera.m_eye = v3f(27.0f, 27.0f, 27.0f);
    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;
    g_projMat = PerspectiveProjectionMatrix((float)currentWindowWidth / currentWindowHeight);

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

    {
        TIMED_SCOPED_BLOCK("Load game assets");
        g_AssetManager.LoadAllAssets(platformFuncs);
        g_AssetManager.InitAssetGraphicsResources(platformFuncs, graphicsCommandStream);
    }

    CreateDefaultGeometry(platformFuncs, graphicsCommandStream);

    CreateGameRenderingResources(platformFuncs, windowWidth, windowHeight);

    // Init view(s)
    DescriptorData_Instance data;
    data.modelMatrix = m4f(1.0f);

    Init(&MainView, MAX_INSTANCES_PER_VIEW);
    uint32 instanceID;
    instanceID = CreateInstance(&MainView, 0);
    data.modelMatrix[3][0] = -8.0f;
    SetInstanceData(&MainView, instanceID, &data);

    instanceID = CreateInstance(&MainView, 1);
    data.modelMatrix[3][0] = -2.5f;
    SetInstanceData(&MainView, instanceID, &data);

    instanceID = CreateInstance(&MainView, 2);
    data.modelMatrix = m4f(0.5f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][0] = 8.0f;
    SetInstanceData(&MainView, instanceID, &data);
    
    instanceID = CreateInstance(&MainView, 2);
    data.modelMatrix = m4f(0.25f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][0] = 8.0f;
    data.modelMatrix[3][2] = 6.0f;
    SetInstanceData(&MainView, instanceID, &data);

    instanceID = CreateInstance(&MainView, 3);
    data.modelMatrix = m4f(7.0f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][1] = 8.0f;
    SetInstanceData(&MainView, instanceID, &data);
    
    instanceID = CreateInstance(&MainView, 3);
    data.modelMatrix = m4f(7.0f);
    data.modelMatrix[3][3] = 1.0f;
    data.modelMatrix[3][1] = 10.0f;
    SetInstanceData(&MainView, instanceID, &data);

    /*for (int i = 0; i < (128 - 6); ++i)
    {
        CreateInstance(&MainView, 2);
    }*/

    CreateAllDescriptors(platformFuncs);
    LoadAllShaders(platformFuncs, windowWidth, windowHeight);

    //CreateAnimatedPolygon();
    {
        Platform::ResourceDesc desc;
        desc.resourceType = Platform::ResourceType::eBuffer1D;

        gameGraphicsData.m_animatedPolygon.numVertices = 25;

        desc.dims = v3ui(gameGraphicsData.m_animatedPolygon.numVertices * sizeof(v4f), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eTransientVertex;
        gameGraphicsData.m_animatedPolygon.vertexBufferHandle = platformFuncs->CreateResource(desc);
        desc.bufferUsage = Platform::BufferUsage::eTransientIndex;
        desc.dims = v3ui((gameGraphicsData.m_animatedPolygon.numVertices - 1) * 3 * sizeof(uint32), 0, 0);
        gameGraphicsData.m_animatedPolygon.indexBufferHandle = platformFuncs->CreateResource(desc);

        // Descriptor - vertex buffer
        Platform::DescriptorLayout descriptorLayout = {};
        descriptorLayout.InitInvalid();
        descriptorLayout.descriptorLayoutParams[1][0].type = Platform::DescriptorType::eSSBO;
        descriptorLayout.descriptorLayoutParams[1][0].amount = 1;

        gameGraphicsData.m_animatedPolygon.descriptor = platformFuncs->CreateDescriptor(&descriptorLayout);

        Platform::DescriptorSetDataHandles descDataHandles[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
        descDataHandles[0].InitInvalid();
        descDataHandles[1].InitInvalid();
        descDataHandles[1].handles[0] = gameGraphicsData.m_animatedPolygon.vertexBufferHandle;
        descDataHandles[2].InitInvalid();        

        Platform::DescriptorHandle descHandles[MAX_DESCRIPTORS_PER_SET] = { Platform::DefaultDescHandle_Invalid, gameGraphicsData.m_animatedPolygon.descriptor, Platform::DefaultDescHandle_Invalid };
        platformFuncs->WriteDescriptor(&descriptorLayout, &descHandles[0], &descDataHandles[0]);

        Platform::GraphicsPipelineParams params;
        params.blendState = Platform::BlendState::eAlphaBlend;
        params.depthState = Platform::DepthState::eTestOnWriteOn;
        params.viewportWidth = windowWidth;
        params.viewportHeight = windowHeight;
        params.framebufferHandle = gameGraphicsData.m_framebufferHandles[eRenderPass_MainView];
        descHandles[0] = gameGraphicsData.m_DescData_Global;
        descHandles[1] = gameGraphicsData.m_animatedPolygon.descriptor;
        descHandles[2] = Platform::DefaultDescHandle_Invalid;
        params.descriptorHandles = descHandles;
        params.numDescriptorHandles = 2;
        gameGraphicsData.m_animatedPolygonShaderHandle = LoadShader(platformFuncs, SHADERS_SPV_PATH "animpoly_vert_glsl.spv", SHADERS_SPV_PATH "animpoly_frag_glsl.spv", &params);
    }

    return 0;
}

extern "C"
GAME_UPDATE(GameUpdate)
{
    graphicsCommandStream->m_numCommands = 0;

    if (!isGameInitted)
    {
        uint32 initResult = GameInit(platformFuncs, graphicsCommandStream, windowWidth, windowHeight);
        if (initResult != 0)
        {
            return initResult;
        }
        isGameInitted = true;
    }

    UpdateAxisVectors(&g_gameCamera);

    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;

    {
        //TIMED_SCOPED_BLOCK("Input manager update - kb/mouse callbacks");
        g_InputManager.UpdateAndDoCallbacks(inputStateDeltas, platformFuncs);
    }

    // Temp test code for deletion of instances
    if (0)
    {
        static uint32 frameCtr = 0;
        static uint32 inst = 0;
        if (frameCtr % 120 == 0)
        {
            DestroyInstance(&MainView, inst);
            ++inst;
            if (inst == 128) inst = 0;
            //CreateInstance(&MainView, 1); // stuff slowly turns into cubes
        }
        ++frameCtr;
    }

    // Update view(s)
    {
        //TIMED_SCOPED_BLOCK("View update");
        MainView.m_viewMatrix = CameraViewMatrix(&g_gameCamera);
        MainView.m_projMatrix = g_projMat;

        Platform::DescriptorSetDataHandles descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0].InitInvalid();
        descriptors[0].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Global;
        descriptors[1].InitInvalid();
        descriptors[1].handles[0] = gameGraphicsData.m_DescDataBufferHandle_Instance;
        Update(&MainView, descriptors, platformFuncs);
    }

    // Clear depth buffer
    {
        Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

        // Transition of depth buffer from layout undefined to transfer_dst (required for clear command)
        command->m_commandType = Platform::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition depth to transfer_dst";
        command->m_imageHandle = gameGraphicsData.m_rtDepthHandle;
        command->m_startLayout = Platform::ImageLayout::eUndefined;
        command->m_endLayout = Platform::ImageLayout::eTransferDst;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Clear depth buffer - before z-prepass
        command->m_commandType = Platform::GraphicsCmd::eClearImage;
        command->debugLabel = "Clear depth buffer";
        command->m_imageHandle = gameGraphicsData.m_rtDepthHandle;
        command->m_clearValue = v4f(1.0f, 0.0f, 0.0f, 0.0f); // depth/stencil clear uses x and y components
        ++graphicsCommandStream->m_numCommands;
        ++command;

        // Transition of depth buffer from transfer dst to depth_attachment_optimal
        command->m_commandType = Platform::GraphicsCmd::eLayoutTransition;
        command->debugLabel = "Transition depth to depth_attachment_optimal";
        command->m_imageHandle = gameGraphicsData.m_rtDepthHandle;
        command->m_startLayout = Platform::ImageLayout::eTransferDst;
        command->m_endLayout = Platform::ImageLayout::eDepthOptimal;
        ++graphicsCommandStream->m_numCommands;
        ++command;
    }

    // Record render commands for view(s)
    {
        //TIMED_SCOPED_BLOCK("Record render pass commands");
        Platform::DescriptorHandle descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
        descriptors[0] = gameGraphicsData.m_DescData_Global;
        descriptors[1] = gameGraphicsData.m_DescData_Instance;

        StartRenderPass(&gameRenderPasses[eRenderPass_ZPrePass], graphicsCommandStream);
        RecordRenderPassCommands(&MainView, &gameRenderPasses[eRenderPass_ZPrePass], graphicsCommandStream, descriptors);
        EndRenderPass(&gameRenderPasses[eRenderPass_ZPrePass], graphicsCommandStream);

        StartRenderPass(&gameRenderPasses[eRenderPass_MainView], graphicsCommandStream);
        RecordRenderPassCommands(&MainView, &gameRenderPasses[eRenderPass_MainView], graphicsCommandStream, descriptors);
        // Draw animated polygon with transient buffers
        {
            // Update animated polygon
            {
                // Map
                void* indexBuf = platformFuncs->MapResource(gameGraphicsData.m_animatedPolygon.indexBufferHandle);
                void* vertexBuf = platformFuncs->MapResource(gameGraphicsData.m_animatedPolygon.vertexBufferHandle);

                // Update
                const uint32 numIndices = ((gameGraphicsData.m_animatedPolygon.numVertices - 1) * 3);
                for (uint32 idx = 0; idx < numIndices; idx += 3)
                {
                    ((uint32*)indexBuf)[idx + 0] = 0;
                    ((uint32*)indexBuf)[idx + 1] = idx / 3 + 1;
                    ((uint32*)indexBuf)[idx + 2] = idx < numIndices - 3 ? idx / 3 + 2 : 1;
                }

                static uint32 frameCtr = 0;
                ++frameCtr;
                const float scale = 2.0f * cosf((float)frameCtr * 0.01f);
                for (uint32 vtx = 0; vtx < gameGraphicsData.m_animatedPolygon.numVertices; ++vtx)
                {
                    if (vtx == 0)
                    {
                        ((v4f*)vertexBuf)[vtx] = v4f(0.0f, 0.0f, 0.0f, 1.0f);
                    }
                    else
                    {
                        const float amt = ((float)(vtx - 1) / (gameGraphicsData.m_animatedPolygon.numVertices - 1)) * (3.14159f * 2.0f);
                        ((v4f*)vertexBuf)[vtx] = v4f(cosf(amt) * scale, sinf(amt) * scale, 0.0f, 1.0f);
                    }
                }

                // Unmap
                platformFuncs->UnmapResource(gameGraphicsData.m_animatedPolygon.indexBufferHandle);
                platformFuncs->UnmapResource(gameGraphicsData.m_animatedPolygon.vertexBufferHandle);
            }

            // Draw call
            Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

            command->m_commandType = Platform::GraphicsCmd::eDrawCall;
            command->debugLabel = "Draw anim poly";
            command->m_numIndices = (gameGraphicsData.m_animatedPolygon.numVertices - 1) * 3;
            command->m_numInstances = 1;
            command->m_indexBufferHandle = gameGraphicsData.m_animatedPolygon.indexBufferHandle;
            command->m_shaderHandle = gameGraphicsData.m_animatedPolygonShaderHandle;

            for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
            {
                command->m_descriptors[i] = Platform::DefaultDescHandle_Invalid;
            }
            command->m_descriptors[0] = gameGraphicsData.m_DescData_Global;
            command->m_descriptors[1] = gameGraphicsData.m_animatedPolygon.descriptor;
            ++graphicsCommandStream->m_numCommands;
        }
        EndRenderPass(&gameRenderPasses[eRenderPass_MainView], graphicsCommandStream);
    }

    // FINAL BLIT TO SCREEN
    Tk::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    command->m_commandType = Platform::GraphicsCmd::eRenderPassBegin;
    command->debugLabel = "Blit to screen";
    command->m_framebufferHandle = Platform::DefaultFramebufferHandle_Invalid;
    command->m_renderWidth = 0;
    command->m_renderHeight = 0;
    ++graphicsCommandStream->m_numCommands;
    ++command;

    command->m_commandType = Platform::GraphicsCmd::eDrawCall;
    command->debugLabel = "Draw default quad";
    command->m_numIndices = DEFAULT_QUAD_NUM_INDICES;
    command->m_numInstances = 1;
    command->m_indexBufferHandle = defaultQuad.m_indexBuffer.gpuBufferHandle;
    command->m_shaderHandle = gameGraphicsData.m_blitShaderHandle;
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        command->m_descriptors[i] = Platform::DefaultDescHandle_Invalid;
    }
    command->m_descriptors[0] = gameGraphicsData.m_swapChainBlitDescHandle;
    command->m_descriptors[1] = defaultQuad.m_descriptor;
    ++graphicsCommandStream->m_numCommands;
    ++command;

    command->m_commandType = Platform::GraphicsCmd::eRenderPassEnd;
    ++graphicsCommandStream->m_numCommands;
    ++command;

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
    for (uint32 uiPass = 0; uiPass < eRenderPass_Max; ++uiPass)
    {
        platformFuncs->DestroyFramebuffer(gameGraphicsData.m_framebufferHandles[uiPass]);
    }
    platformFuncs->DestroyResource(gameGraphicsData.m_rtColorHandle);
    platformFuncs->DestroyResource(gameGraphicsData.m_rtDepthHandle);

    DestroyShaders(platformFuncs);
}

extern "C"
GAME_WINDOW_RESIZE(GameWindowResize)
{
    currentWindowWidth = newWindowWidth;
    currentWindowHeight = newWindowHeight;
    DestroyWindowResizeDependentResources(platformFuncs);

    // Gameplay stuff
    g_projMat = PerspectiveProjectionMatrix((float)currentWindowWidth / currentWindowHeight);

    CreateGameRenderingResources(platformFuncs, newWindowWidth, newWindowHeight);
    WriteSwapChainBlitResources(platformFuncs);

    LoadAllShaders(platformFuncs, newWindowWidth, newWindowHeight);
}

extern "C"
GAME_DESTROY(GameDestroy)
{
    if (isGameInitted)
    {
        DestroyWindowResizeDependentResources(platformFuncs);
        DestroyDescriptors(platformFuncs);

        DestroyDefaultGeometry(platformFuncs);
        DestroyDefaultGeometryVertexBufferDescriptor(defaultQuad, platformFuncs);

        // Destroy assets
        g_AssetManager.DestroyAllMeshData(platformFuncs);
        g_AssetManager.DestroyAllTextureData(platformFuncs);

        if (isMultiplayer && connectedToServer)
        {
            platformFuncs->EndNetworkConnection();
        }

        g_AssetManager.FreeMemory();
        FreeShaderBytecodeMemory();
    }

    #if defined(MEM_TRACKING)
    _CrtDumpMemoryLeaks();
    #endif
}
