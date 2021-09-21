#include "Platform/ShaderManager.h"

#ifdef _SHADERS_SPV_DIR
#define SHADERS_SPV_PATH STRINGIFY(_SHADERS_SPV_DIR)
#else
//#define SHADERS_SPV_PATH "..\\Shaders\\spv\\"
#endif

static const uint32 totalShaderBytecodeMaxSizeInBytes = 1024 * 1024 * 100;

namespace Tk
{
namespace Platform
{

ShaderManager g_ShaderManager = {};

void ShaderManager::Startup()
{
    shaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);
}

void ShaderManager::Shutdown()
{
    shaderBytecodeAllocator.ExplicitFree();
}

void ShaderManager::RecreateWindowDependentResources(const Tk::Platform::PlatformAPIFuncs* platformFuncs, uint32 newWindowWidth, uint32 newWindowHeight)
{
    //platformFuncs->DestroyGraphicsPipeline(SHADER_ID_SWAP_CHAIN_BLIT);

    CreateAllRenderPasses(platformFuncs);
    // TODO: don't reload the shader every time we resize, need to be able to reference existing bytecode... which we do already store
    LoadAllShaders(platformFuncs, newWindowWidth, newWindowHeight);
}

bool ShaderManager::LoadShader(const Tk::Platform::PlatformAPIFuncs* platformFuncs,
    const char* vertexShaderFileName, const char* fragmentShaderFileName,
    uint32 shaderID, uint32 viewportWidth, uint32 viewportHeight, uint32 renderPassID,
    uint32* descLayouts, uint32 numDescLayouts)
{
    uint8* vertexShaderBuffer = nullptr;
    uint8* fragmentShaderBuffer = nullptr;
    uint32 vertexShaderFileSize = 0, fragmentShaderFileSize = 0;

    if (vertexShaderFileName)
    {
        vertexShaderFileSize = platformFuncs->GetFileSize(vertexShaderFileName);
        vertexShaderBuffer = shaderBytecodeAllocator.Alloc(vertexShaderFileSize, 1);
        TINKER_ASSERT(vertexShaderBuffer);
        platformFuncs->ReadEntireFile(vertexShaderFileName, vertexShaderFileSize, vertexShaderBuffer);
    }

    if (fragmentShaderFileName)
    {
        fragmentShaderFileSize = platformFuncs->GetFileSize(fragmentShaderFileName);
        fragmentShaderBuffer = shaderBytecodeAllocator.Alloc(fragmentShaderFileSize, 1);
        TINKER_ASSERT(fragmentShaderBuffer);
        platformFuncs->ReadEntireFile(fragmentShaderFileName, fragmentShaderFileSize, fragmentShaderBuffer);
    }

    const bool created = platformFuncs->CreateGraphicsPipeline(
        vertexShaderBuffer, vertexShaderFileSize,
        fragmentShaderBuffer, fragmentShaderFileSize,
        shaderID, viewportWidth, viewportHeight, renderPassID, descLayouts, numDescLayouts);
    return created;
}

void ShaderManager::LoadAllShaders(const Tk::Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    shaderBytecodeAllocator.ExplicitFree();
    shaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);

    bool bOk = false;

    // Shaders
    const uint32 numShaderFilepaths = 7;
    const char* shaderFilePaths[numShaderFilepaths] =
    {
        SHADERS_SPV_PATH "blit_vert_glsl.spv",
        SHADERS_SPV_PATH "blit_frag_glsl.spv",
        SHADERS_SPV_PATH "basic_vert_glsl.spv",
        SHADERS_SPV_PATH "basic_frag_glsl.spv",
        SHADERS_SPV_PATH "animpoly_vert_glsl.spv",
        SHADERS_SPV_PATH "animpoly_frag_glsl.spv",
        SHADERS_SPV_PATH "basic_virtualTex_frag_glsl.spv",
    };

    uint32 descLayouts[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = DESCLAYOUT_ID_MAX;

    // Swap chain blit
    descLayouts[0] = DESCLAYOUT_ID_SWAP_CHAIN_BLIT_TEX;
    descLayouts[1] = DESCLAYOUT_ID_SWAP_CHAIN_BLIT_VBS;
    bOk = LoadShader(platformFuncs, shaderFilePaths[0], shaderFilePaths[1], SHADER_ID_SWAP_CHAIN_BLIT, windowWidth, windowHeight, RENDERPASS_ID_SWAP_CHAIN_BLIT, descLayouts, 2);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = DESCLAYOUT_ID_MAX;

    // ZPrepass
    descLayouts[0] = DESCLAYOUT_ID_VIEW_GLOBAL;
    descLayouts[1] = DESCLAYOUT_ID_ASSET_INSTANCE;
    descLayouts[2] = DESCLAYOUT_ID_ASSET_VBS;
    bOk = LoadShader(platformFuncs, shaderFilePaths[2], nullptr, SHADER_ID_BASIC_ZPrepass, windowWidth, windowHeight, RENDERPASS_ID_ZPrepass, descLayouts, 3);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = DESCLAYOUT_ID_MAX;

    // Main view
    descLayouts[0] = DESCLAYOUT_ID_VIEW_GLOBAL;
    descLayouts[1] = DESCLAYOUT_ID_ASSET_INSTANCE;
    descLayouts[2] = DESCLAYOUT_ID_ASSET_VBS;
    bOk = LoadShader(platformFuncs, shaderFilePaths[2], shaderFilePaths[3], SHADER_ID_BASIC_MainView, windowWidth, windowHeight, RENDERPASS_ID_MainView, descLayouts, 3);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = DESCLAYOUT_ID_MAX;

    // Animated poly
    descLayouts[0] = DESCLAYOUT_ID_VIEW_GLOBAL;
    descLayouts[1] = DESCLAYOUT_ID_ANIMPOLY_VBS;
    bOk = LoadShader(platformFuncs, shaderFilePaths[4], shaderFilePaths[5], SHADER_ID_ANIMATEDPOLY_MainView, windowWidth, windowHeight, RENDERPASS_ID_MainView, descLayouts, 2);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = DESCLAYOUT_ID_MAX;

    // Virtual texture, basic shader
    descLayouts[0] = DESCLAYOUT_ID_VIRTUAL_TEXTURE;
    descLayouts[1] = DESCLAYOUT_ID_TERRAIN_DATA;
    bOk = LoadShader(platformFuncs, shaderFilePaths[2], shaderFilePaths[6], SHADER_ID_BASIC_VirtualTexture, windowWidth, windowHeight, RENDERPASS_ID_MainView, descLayouts, 2);
    TINKER_ASSERT(bOk);
}

void ShaderManager::CreateAllRenderPasses(const Tk::Platform::PlatformAPIFuncs* platformFuncs)
{
    bool bOk = false;
    // Render passes
    // TODO: make the render pass creation stuff more controllable by the app

    // NOTE: this render pass is created inside the vulkan init code
    // color, no depth
    //bOk = platformFuncs->CreateRenderPass(RENDERPASS_ID_SWAP_CHAIN_BLIT, 1, ImageFormat::RGBA8_SRGB, ImageLayout::eUndefined, ImageLayout::ePresent, ImageFormat::Invalid);
    //TINKER_ASSERT(bOk);

    // depth, no color
    bOk = platformFuncs->CreateRenderPass(RENDERPASS_ID_ZPrepass, 0, ImageFormat::Invalid, ImageLayout::eUndefined, ImageLayout::eUndefined, ImageFormat::Depth_32F);
    TINKER_ASSERT(bOk);

    // color, depth
    bOk = platformFuncs->CreateRenderPass(RENDERPASS_ID_MainView, 1, ImageFormat::RGBA8_SRGB, ImageLayout::eUndefined, ImageLayout::eShaderRead, ImageFormat::Depth_32F);
    TINKER_ASSERT(bOk);
}

void ShaderManager::LoadAllShaderResources(const Tk::Platform::PlatformAPIFuncs* platformFuncs, uint32 windowWidth, uint32 windowHeight)
{
    bool bOk = false;

    // Descriptor layouts
    Platform::DescriptorLayout descriptorLayout = {};

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eSampledImage;
    descriptorLayout.params[0].amount = 1;
    bOk = platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_SWAP_CHAIN_BLIT_TEX, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Platform::DescriptorType::eSSBO;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Platform::DescriptorType::eSSBO;
    descriptorLayout.params[2].amount = 1;
    bOk = platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_SWAP_CHAIN_BLIT_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eBuffer;
    descriptorLayout.params[0].amount = 1;
    bOk = platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_VIEW_GLOBAL, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eBuffer;
    descriptorLayout.params[0].amount = 1;
    bOk = platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_ASSET_INSTANCE, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Platform::DescriptorType::eSSBO;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Platform::DescriptorType::eSSBO;
    descriptorLayout.params[2].amount = 1;
    bOk = platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_ASSET_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    bOk = platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_ANIMPOLY_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eSampledImage;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Platform::DescriptorType::eBuffer;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Platform::DescriptorType::eSampledImage;
    descriptorLayout.params[2].amount = 1;
    bOk = platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_VIRTUAL_TEXTURE, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eBuffer;
    descriptorLayout.params[0].amount = 1;
    bOk = platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_TERRAIN_DATA, &descriptorLayout);
    TINKER_ASSERT(bOk);
    //-----

    CreateAllRenderPasses(platformFuncs);

    LoadAllShaders(platformFuncs, windowWidth, windowHeight);
}

}
}
