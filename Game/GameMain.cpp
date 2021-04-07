#include "PlatformGameAPI.h"
#include "Core/Allocators.h"
#include "Core/Math/VectorTypes.h"
#include "Core/Containers/RingBuffer.h"
#include "Core/FileIO/FileLoading.h"
#include "Core/Utilities/ScopedTimer.h"
#include "GameGraphicsTypes.h"
#include "ShaderLoading.h"
#include "GameRenderPass.h"
#include "AssetManager.h"
#include "Camera.h"

#ifdef _SCRIPTS_DIR
#define SCRIPTS_PATH STRINGIFY(_SCRIPTS_DIR)
#else
//#define SCRIPTS_PATH "..\\Scripts\\"
#endif

#include <cstring>
#include <string.h>

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
    // buffer handles
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
    { DefaultResHandle_Invalid },
    // positions
    v4f(-1.0f, -1.0f, 0.0f, 1.0f),
    v4f(1.0f, -1.0f, 0.0f, 1.0f),
    v4f(-1.0f, 1.0f, 0.0f, 1.0f),
    v4f(1.0f, 1.0f, 0.0f, 1.0f),
    //uvs
    v2f(0.0f, 0.0f),
    v2f(0.0f, 1.0f),
    v2f(0.0f, 1.0f),
    v2f(1.0f, 1.0f),
    // normals
    v3f(0.0f, 0.0f, 1.0f),
    v3f(0.0f, 0.0f, 1.0f),
    v3f(0.0f, 0.0f, 1.0f),
    v3f(0.0f, 0.0f, 1.0f),
    // indices
    0, 1, 2, 2, 1, 3
};

static GameGraphicsData gameGraphicsData = {};
static GameRenderPass gameRenderPasses[eRenderPass_Max] = {};

static bool isGameInitted = false;
static const bool isMultiplayer = false;
static bool connectedToServer = false;

uint32 currentWindowWidth, currentWindowHeight;

typedef struct input_state
{
    Platform::KeycodeState keyCodes[Platform::Keycode::eMax];
} InputState;
static InputState currentInputState = {};
static InputState previousInputState = {};

typedef struct descriptor_instance_data
{
    alignas(16) m4f modelMatrix;
    alignas(16) m4f viewProj;
} DescriptorInstanceData;

static VirtualCamera g_gameCamera = {};

// TODO: remove me
#include <chrono>
void UpdateDescriptorState(const Platform::PlatformAPIFuncs* platformFuncs)
{
    // TODO: remove this, is just for testing
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    float scale = 1.0f;
    DescriptorInstanceData instanceData = {};
    instanceData.modelMatrix = m4f(scale);
    //instanceData.modelMatrix[1][1] = 1.0f;
    instanceData.modelMatrix[3][3] = 1.0f;
    instanceData.viewProj = g_projMat * CameraViewMatrix(&g_gameCamera);
    gameGraphicsData.m_modelMatrixBufferMemPtr1 = platformFuncs->MapResource(gameGraphicsData.m_modelMatrixBufferHandle1);
    memcpy(gameGraphicsData.m_modelMatrixBufferMemPtr1, &instanceData, sizeof(instanceData));
    platformFuncs->UnmapResource(gameGraphicsData.m_modelMatrixBufferHandle1);
}

void LoadAllShaders(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    ResetShaderBytecodeAllocator();

    Platform::DescriptorLayout mainDrawDescriptorLayout = {};
    Platform::InitDescLayout(&mainDrawDescriptorLayout);
    Platform::GraphicsPipelineParams params;
    params.blendState = Platform::BlendState::eInvalid; // no color attachment
    params.depthState = Platform::DepthState::eTestOnWriteOn; // Write to depth
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.framebufferHandle = gameGraphicsData.m_framebufferHandles[eRenderPass_ZPrePass];
    params.descriptorHandle = gameGraphicsData.m_modelMatrixDescHandle1;
    gameGraphicsData.m_shaderHandles[eRenderPass_ZPrePass] = LoadShader(platformFuncs, SHADERS_SPV_PATH "basic_vert_glsl.spv", nullptr, &params);

    params.framebufferHandle = gameGraphicsData.m_framebufferHandles[eRenderPass_MainView];
    params.blendState = Platform::BlendState::eAlphaBlend; // Default alpha blending for now
    params.depthState = Platform::DepthState::eTestOnWriteOff; // Read depth buffer, don't write to it
    gameGraphicsData.m_shaderHandles[eRenderPass_MainView] = LoadShader(platformFuncs, SHADERS_SPV_PATH "basic_vert_glsl.spv", SHADERS_SPV_PATH "basic_frag_glsl.spv", &params);

    // TODO: dont want to have to do this
    // Set data for render pass
    gameRenderPasses[eRenderPass_ZPrePass].shader = gameGraphicsData.m_shaderHandles[eRenderPass_ZPrePass];
    Platform::DescriptorSetDescHandles descriptors[MAX_DESCRIPTOR_SETS_PER_SHADER];
    InitDescSetDescHandles(descriptors);
    descriptors[0].handles[0] = gameGraphicsData.m_modelMatrixDescHandle1;
    memcpy(gameRenderPasses[eRenderPass_ZPrePass].descriptors, descriptors, sizeof(descriptors));

    gameRenderPasses[eRenderPass_MainView].shader = gameGraphicsData.m_shaderHandles[eRenderPass_MainView];
    memcpy(gameRenderPasses[eRenderPass_MainView].descriptors, descriptors, sizeof(descriptors));

    params.blendState = Platform::BlendState::eAlphaBlend;
    params.depthState = Platform::DepthState::eOff;
    params.viewportWidth = windowWidth;
    params.viewportHeight = windowHeight;
    params.framebufferHandle = DefaultFramebufferHandle_Invalid;
    params.descriptorHandle = gameGraphicsData.m_swapChainBlitDescHandle;
    gameGraphicsData.m_blitShaderHandle = LoadShader(platformFuncs, SHADERS_SPV_PATH "blit_vert_glsl.spv", SHADERS_SPV_PATH "blit_frag_glsl.spv", &params);
}

void DestroyShaders(const Platform::PlatformAPIFuncs* platformFuncs)
{
    for (uint32 uiPass = 0; uiPass < eRenderPass_Max; ++uiPass)
    {
        platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_shaderHandles[uiPass]);
        gameGraphicsData.m_shaderHandles[uiPass] = DefaultShaderHandle_Invalid;
    }

    platformFuncs->DestroyGraphicsPipeline(gameGraphicsData.m_blitShaderHandle);
    gameGraphicsData.m_blitShaderHandle = DefaultShaderHandle_Invalid;
}

void DestroyDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    platformFuncs->DestroyDescriptor(gameGraphicsData.m_swapChainBlitDescHandle);
    gameGraphicsData.m_swapChainBlitDescHandle = DefaultDescHandle_Invalid;

    platformFuncs->DestroyDescriptor(gameGraphicsData.m_modelMatrixDescHandle1);
    gameGraphicsData.m_modelMatrixDescHandle1 = DefaultDescHandle_Invalid;
    platformFuncs->DestroyResource(gameGraphicsData.m_modelMatrixBufferHandle1);
    gameGraphicsData.m_modelMatrixBufferHandle1 = DefaultResHandle_Invalid;

    platformFuncs->DestroyAllDescriptors(); // TODO: this is not a good API and should be per-pool or something
}

void CreateAllDescriptors(const Platform::PlatformAPIFuncs* platformFuncs)
{
    // Swap chain blit
    Platform::DescriptorLayout blitDescriptorLayout = {};
    Platform::InitDescLayout(&blitDescriptorLayout);
    blitDescriptorLayout.descriptorLayoutParams[0][0].type = Platform::DescriptorType::eSampledImage;
    blitDescriptorLayout.descriptorLayoutParams[0][0].amount = 1;
    gameGraphicsData.m_swapChainBlitDescHandle = platformFuncs->CreateDescriptor(&blitDescriptorLayout);

    Platform::DescriptorSetDataHandles blitHandles = {};
    blitHandles.handles[0] = gameGraphicsData.m_rtColorHandle;
    platformFuncs->WriteDescriptor(&blitDescriptorLayout, gameGraphicsData.m_swapChainBlitDescHandle, &blitHandles);

    // Model matrix
    ResourceDesc desc;
    desc.resourceType = Platform::ResourceType::eBuffer1D;
    desc.dims = v3ui(sizeof(DescriptorInstanceData), 0, 0);
    desc.bufferUsage = Platform::BufferUsage::eUniform;
    gameGraphicsData.m_modelMatrixBufferHandle1 = platformFuncs->CreateResource(desc);

    Platform::DescriptorLayout instanceDataDescriptorLayout = {};
    Platform::InitDescLayout(&instanceDataDescriptorLayout);
    instanceDataDescriptorLayout.descriptorLayoutParams[0][0].type = Platform::DescriptorType::eBuffer;
    instanceDataDescriptorLayout.descriptorLayoutParams[0][0].amount = 1;
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

void CreateDefaultGeometry(const Platform::PlatformAPIFuncs* platformFuncs, Platform::GraphicsCommandStream* graphicsCommandStream)
{
    // Default Quad
    {
        ResourceDesc desc;
        desc.resourceType = Platform::ResourceType::eBuffer1D;

        // Positions
        desc.dims = v3ui(sizeof(defaultQuad.m_points), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eVertex;
        defaultQuad.m_positionBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_Pos = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_Pos = platformFuncs->MapResource(stagingBufferHandle_Pos);

        // UVs
        desc.dims = v3ui(sizeof(defaultQuad.m_uvs), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eVertex;
        defaultQuad.m_uvBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_UV = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_UV = platformFuncs->MapResource(stagingBufferHandle_UV);

        // Normals
        desc.dims = v3ui(sizeof(defaultQuad.m_normals), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eVertex;
        defaultQuad.m_normalBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_Norm = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_Norm = platformFuncs->MapResource(stagingBufferHandle_Norm);

        // Indices
        desc.dims = v3ui(sizeof(defaultQuad.m_indices), 0, 0);
        desc.bufferUsage = Platform::BufferUsage::eIndex;
        defaultQuad.m_indexBuffer.gpuBufferHandle = platformFuncs->CreateResource(desc);

        desc.bufferUsage = Platform::BufferUsage::eStaging;
        ResourceHandle stagingBufferHandle_Idx = platformFuncs->CreateResource(desc);
        void* stagingBufferMemPtr_Idx = platformFuncs->MapResource(stagingBufferHandle_Idx);

        // Memcpy into staging buffers
        memcpy(stagingBufferMemPtr_Pos, defaultQuad.m_points, sizeof(defaultQuad.m_points));
        memcpy(stagingBufferMemPtr_UV, defaultQuad.m_uvs, sizeof(defaultQuad.m_uvs));
        memcpy(stagingBufferMemPtr_Norm, defaultQuad.m_normals, sizeof(defaultQuad.m_normals));
        memcpy(stagingBufferMemPtr_Idx, defaultQuad.m_indices, sizeof(defaultQuad.m_indices));

        // Do GPU buffer copies
        Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];
        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx Pos Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_points);
        command->m_dstBufferHandle = defaultQuad.m_positionBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_Pos;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx UV Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_uvs);
        command->m_dstBufferHandle = defaultQuad.m_uvBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_UV;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx Norm Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_normals);
        command->m_dstBufferHandle = defaultQuad.m_normalBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_Norm;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        command->m_commandType = (uint32)Platform::GraphicsCmd::eMemTransfer;
        command->debugLabel = "Update Default Quad Vtx Idx Buf";
        command->m_sizeInBytes = sizeof(defaultQuad.m_indices);
        command->m_dstBufferHandle = defaultQuad.m_indexBuffer.gpuBufferHandle;
        command->m_srcBufferHandle = stagingBufferHandle_Idx;
        ++graphicsCommandStream->m_numCommands;
        ++command;

        platformFuncs->SubmitCmdsImmediate(graphicsCommandStream);
        graphicsCommandStream->m_numCommands = 0; // reset the cmd counter for the stream

        // Unmap + destroy resources
        platformFuncs->UnmapResource(stagingBufferHandle_Pos);
        platformFuncs->UnmapResource(stagingBufferHandle_UV);
        platformFuncs->UnmapResource(stagingBufferHandle_Norm);
        platformFuncs->UnmapResource(stagingBufferHandle_Idx);

        platformFuncs->DestroyResource(stagingBufferHandle_Pos);
        platformFuncs->DestroyResource(stagingBufferHandle_UV);
        platformFuncs->DestroyResource(stagingBufferHandle_Norm);
        platformFuncs->DestroyResource(stagingBufferHandle_Idx);
    }
}

void DestroyDefaultGeometry(const Platform::PlatformAPIFuncs* platformFuncs)
{
    // Default quad
    platformFuncs->DestroyResource(defaultQuad.m_positionBuffer.gpuBufferHandle);
    platformFuncs->DestroyResource(defaultQuad.m_uvBuffer.gpuBufferHandle);
    platformFuncs->DestroyResource(defaultQuad.m_normalBuffer.gpuBufferHandle);
    platformFuncs->DestroyResource(defaultQuad.m_indexBuffer.gpuBufferHandle);
}

void CreateGameRenderingResources(const Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    ResourceDesc desc;
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

void ProcessInputState(const Platform::InputStateDeltas* inputStateDeltas, const Platform::PlatformAPIFuncs* platformFuncs)
{
    previousInputState = currentInputState;

    for (uint32 uiKeycode = 0; uiKeycode < Platform::Keycode::eMax; ++uiKeycode)
    {
        if (inputStateDeltas->keyCodes[uiKeycode].numStateChanges > 0)
        {
            currentInputState.keyCodes[uiKeycode].isDown = inputStateDeltas->keyCodes[uiKeycode].isDown;
            currentInputState.keyCodes[uiKeycode].numStateChanges += inputStateDeltas->keyCodes[uiKeycode].numStateChanges;
            // TODO: reset number of state changes at some point, every second or every several frames or something
        }
    }

    for (uint32 uiKeycode = 0; uiKeycode < Platform::Keycode::eMax; ++uiKeycode)
    {
        switch (uiKeycode)
        {
            case Platform::Keycode::eF10:
            {
                // Handle the initial downpress once and nothing more
                if (currentInputState.keyCodes[uiKeycode].isDown && !previousInputState.keyCodes[uiKeycode].isDown)
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

#include "Core/Raytracing/AccelStructures/Octree.h"
Raytracing::Octree* octree = nullptr;
extern "C"
GAME_UPDATE(GameUpdate)
{
    graphicsCommandStream->m_numCommands = 0;

    if (!isGameInitted)
    {
        TIMED_SCOPED_BLOCK("Game Init");

        g_gameCamera.m_ref = v3f(0.0f, 0.0f, 0.0f);
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

        CreateAllDescriptors(platformFuncs);
        LoadAllShaders(platformFuncs, windowWidth, windowHeight);

        // Octree test
        {
            const MeshAttributeData& data = g_AssetManager.GetMeshAttrDataByID(2);
            uint32 numVerts = data.m_numVertices / 4;
            v3f* triData = new v3f[numVerts * sizeof(v3f)];
            for (uint32 i = 0; i < numVerts; ++i)
            {
                const v4f& ptVec4 = ((v4f*)data.m_vertexBufferData)[i];
                triData[i] = v3f(ptVec4.x, ptVec4.y, ptVec4.z);
            }
            octree = Raytracing::CreateEmptyOctree();
            Raytracing::BuildOctree(octree, triData, numVerts);

            // TODO: write a raytracer to see if the accel structure works
            // raytrace an image, write it out
            {
                const uint32 width = 512;
                const uint32 height = 512;
                uint32* img = new uint32[width * height];
                memset(img, 0, width * height * 4);

                float aspect = (float)width / height;

                for (uint32 px = 0; px < width; ++px)
                {
                    for (uint32 py = 0; py < height; ++py)
                    {
                        v3f rayOrigin = v3f();
                        v3f rayDir = v3f();
                        
                        // Cast ray from camera
                        {
                            v4f coord = v4f((float)px, (float)py, 1.0, 1.0);
                            coord.x /= width;
                            coord.y /= height;
                            coord.x = coord.x * 2 - 1;
                            coord.y = coord.y * 2 - 1;

                            v3f camEye = v3f(27, 27, 27);
                            v3f camRef = g_gameCamera.m_ref;
                            rayOrigin = camEye;
                            v3f look = camRef - camEye;
                            float len = Length(look);
                            Normalize(look);
                            v3f right = Cross(look, v3f(0, 0, 1));
                            Normalize(right);
                            v3f up = Cross(right, look);
                            Normalize(up);

                            v3f H = right * len * tan(fovy * 0.5f) * aspect;
                            v3f V = up * len * tan(fovy * 0.5f);

                            v3f screenPt = camRef + H * coord.x + V * coord.y;
                            rayDir = screenPt - camEye;
                            Normalize(rayDir);
                        }

                        uint8 channel = 255;
                        // intersect with octree
                        {
                            Raytracing::Ray ray = Raytracing::Ray();
                            ray.origin = rayOrigin;
                            ray.dir = rayDir;
                            Raytracing::Intersection isx = Raytracing::IntersectRay(octree, ray);

                            if (isx.t > 0.0f)
                            {
                                // TODO: use bary coord to interpolate vertex normals
                                float lambert = 0.5f;
                                channel = (uint8)(lambert * 255);
                            }
                            else
                            {
                                // No intersection
                            }
                        }

                        img[py * 512 + px] = (uint32)(channel | (channel << 8) | (channel << 16) | (channel << 24));
                    }
                }

                // Output image



                delete img;
            }


            delete triData;
        }

        isGameInitted = true;
    }

    // TODO: move this
    // TODO: pass time data into this function
    // Animate camera
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    g_gameCamera.m_eye = v3f(cosf(time) * 27.0f, sinf(time) * 27.0f, 27.0f);

    currentWindowWidth = windowWidth;
    currentWindowHeight = windowHeight;

    ProcessInputState(inputStateDeltas, platformFuncs);
    UpdateDescriptorState(platformFuncs);

    // Record buffer update commands
    // TODO: have dynamic assets?
    /*for (uint32 uiAssetID = 0; uiAssetID < g_AssetManager.m_numMeshAssets; ++uiAssetID)
    {
        DynamicMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(uiAssetID);

        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_positionBuffer, meshData->m_numIndices * sizeof(v4f), "Update Asset Vtx Pos Buf");
        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_uvBuffer, meshData->m_numIndices * sizeof(v2f), "Update Asset Vtx Uv Buf");
        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_normalBuffer, meshData->m_numIndices * sizeof(v3f), "Update Asset Vtx Norm Buf");
        UpdateDynamicBufferCommand(graphicsCommands, &meshData->m_indexBuffer, meshData->m_numIndices * sizeof(uint32), "Update Asset Vtx Idx Buf");
    }*/

    {
        Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

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

    // Draw calls
    for (uint32 uiPass = 0; uiPass < eRenderPass_Max; ++uiPass)
    {
        RecordAllCommands(&gameRenderPasses[uiPass], platformFuncs, graphicsCommandStream);
    }

    // FINAL BLIT TO SCREEN
    Tinker::Platform::GraphicsCommand* command = &graphicsCommandStream->m_graphicsCommands[graphicsCommandStream->m_numCommands];

    // Blit to screen
    command->m_commandType = Platform::GraphicsCmd::eRenderPassBegin;
    command->debugLabel = "Blit to screen";
    command->m_framebufferHandle = DefaultFramebufferHandle_Invalid;
    command->m_renderWidth = 0;
    command->m_renderHeight = 0;
    ++graphicsCommandStream->m_numCommands;
    ++command;

    command->m_commandType = Platform::GraphicsCmd::eDrawCall;
    command->debugLabel = "Draw default quad";
    command->m_numIndices = DEFAULT_QUAD_NUM_INDICES;
    command->m_positionBufferHandle = defaultQuad.m_positionBuffer.gpuBufferHandle;
    command->m_uvBufferHandle = defaultQuad.m_uvBuffer.gpuBufferHandle;
    command->m_normalBufferHandle = defaultQuad.m_normalBuffer.gpuBufferHandle;
    command->m_indexBufferHandle = defaultQuad.m_indexBuffer.gpuBufferHandle;
    command->m_shaderHandle = gameGraphicsData.m_blitShaderHandle;
    Platform::InitDescSetDescHandles(command->m_descriptors);
    command->m_descriptors[0].handles[0] = gameGraphicsData.m_swapChainBlitDescHandle;
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
    DestroyDescriptors(platformFuncs);
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
    CreateAllDescriptors(platformFuncs);
    LoadAllShaders(platformFuncs, newWindowWidth, newWindowHeight);
}

extern "C"
GAME_DESTROY(GameDestroy)
{
    if (isGameInitted)
    {
        DestroyWindowResizeDependentResources(platformFuncs);

        DestroyDefaultGeometry(platformFuncs);

        // Game graphics
        for (uint32 uiAssetID = 0; uiAssetID < g_AssetManager.m_numMeshAssets; ++uiAssetID)
        {
            StaticMeshData* meshData = g_AssetManager.GetMeshGraphicsDataByID(uiAssetID);

            platformFuncs->DestroyResource(meshData->m_positionBuffer.gpuBufferHandle);
            platformFuncs->DestroyResource(meshData->m_uvBuffer.gpuBufferHandle);
            platformFuncs->DestroyResource(meshData->m_normalBuffer.gpuBufferHandle);
            platformFuncs->DestroyResource(meshData->m_indexBuffer.gpuBufferHandle);
        }

        for (uint32 uiAssetID = 0; uiAssetID < g_AssetManager.m_numTextureAssets; ++uiAssetID)
        {
            ResourceHandle textureHandle = g_AssetManager.GetTextureGraphicsDataByID(uiAssetID);
            platformFuncs->DestroyResource(textureHandle);
        }

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
