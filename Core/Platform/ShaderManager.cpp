#include "Platform/ShaderManager.h"
#include "Graphics/Common/GraphicsCommon.h"

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

void ShaderManager::ReloadShaders(uint32 newWindowWidth, uint32 newWindowHeight)
{
    Graphics::DestroyAllPSOPerms();
    LoadAllShaders(newWindowWidth, newWindowHeight);
}

void ShaderManager::CreateWindowDependentResources(uint32 newWindowWidth, uint32 newWindowHeight)
{
    CreateAllRenderPasses();
    // TODO: don't reload the shader every time we resize, need to be able to reference existing bytecode... which we do already store
    LoadAllShaders(newWindowWidth, newWindowHeight);
}

bool ShaderManager::LoadShader(const char* vertexShaderFileName, const char* fragmentShaderFileName,
    uint32 shaderID, uint32 viewportWidth, uint32 viewportHeight, uint32 renderPassID,
    uint32* descLayouts, uint32 numDescLayouts)
{
    uint8* vertexShaderBuffer = nullptr;
    uint8* fragmentShaderBuffer = nullptr;
    uint32 vertexShaderFileSize = 0, fragmentShaderFileSize = 0;

    if (vertexShaderFileName)
    {
        vertexShaderFileSize = GetEntireFileSize(vertexShaderFileName);
        vertexShaderBuffer = shaderBytecodeAllocator.Alloc(vertexShaderFileSize, 1);
        TINKER_ASSERT(vertexShaderBuffer);
        ReadEntireFile(vertexShaderFileName, vertexShaderFileSize, vertexShaderBuffer);
    }

    if (fragmentShaderFileName)
    {
        fragmentShaderFileSize = GetEntireFileSize(fragmentShaderFileName);
        fragmentShaderBuffer = shaderBytecodeAllocator.Alloc(fragmentShaderFileSize, 1);
        TINKER_ASSERT(fragmentShaderBuffer);
        ReadEntireFile(fragmentShaderFileName, fragmentShaderFileSize, fragmentShaderBuffer);
    }

    const bool created = Tk::Platform::Graphics::CreateGraphicsPipeline(
        vertexShaderBuffer, vertexShaderFileSize,
        fragmentShaderBuffer, fragmentShaderFileSize,
        shaderID, viewportWidth, viewportHeight, renderPassID, descLayouts, numDescLayouts);
    return created;
}

void ShaderManager::LoadAllShaders(uint32 windowWidth, uint32 windowHeight)
{
    shaderBytecodeAllocator.ExplicitFree();
    shaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);

    bool bOk = false;

    // Shaders
    const uint32 numShaderFilepaths = 8;
    const char* shaderFilePaths[numShaderFilepaths] =
    {
        SHADERS_SPV_PATH "blit_vert_glsl.spv",
        SHADERS_SPV_PATH "blit_frag_glsl.spv",
        SHADERS_SPV_PATH "basic_vert_glsl.spv",
        SHADERS_SPV_PATH "basic_frag_glsl.spv",
        SHADERS_SPV_PATH "animpoly_vert_glsl.spv",
        SHADERS_SPV_PATH "animpoly_frag_glsl.spv",
        SHADERS_SPV_PATH "basic_noModelMat_vert_glsl.spv",
        SHADERS_SPV_PATH "basic_virtualTex_frag_glsl.spv",
    };

    uint32 descLayouts[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;

    // Swap chain blit
    descLayouts[0] = Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_TEX;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_VBS;
    bOk = LoadShader(shaderFilePaths[0], shaderFilePaths[1], Graphics::SHADER_ID_SWAP_CHAIN_BLIT, windowWidth, windowHeight, Graphics::RENDERPASS_ID_SWAP_CHAIN_BLIT, descLayouts, 2);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;

    // ZPrepass
    descLayouts[0] = Graphics::DESCLAYOUT_ID_VIEW_GLOBAL;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_ASSET_INSTANCE;
    descLayouts[2] = Graphics::DESCLAYOUT_ID_ASSET_VBS;
    bOk = LoadShader(shaderFilePaths[2], nullptr, Graphics::SHADER_ID_BASIC_ZPrepass, windowWidth, windowHeight, Graphics::RENDERPASS_ID_ZPrepass, descLayouts, 3);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;

    // Main view
    descLayouts[0] = Graphics::DESCLAYOUT_ID_VIEW_GLOBAL;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_ASSET_INSTANCE;
    descLayouts[2] = Graphics::DESCLAYOUT_ID_ASSET_VBS;
    bOk = LoadShader(shaderFilePaths[2], shaderFilePaths[3], Graphics::SHADER_ID_BASIC_MainView, windowWidth, windowHeight, Graphics::RENDERPASS_ID_MainView, descLayouts, 3);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;

    // Animated poly
    descLayouts[0] = Graphics::DESCLAYOUT_ID_VIEW_GLOBAL;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_POSONLY_VBS;
    bOk = LoadShader(shaderFilePaths[4], shaderFilePaths[5], Graphics::SHADER_ID_ANIMATEDPOLY_MainView, windowWidth, windowHeight, Graphics::RENDERPASS_ID_MainView, descLayouts, 2);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;

    // Virtual texture, basic shader
    descLayouts[0] = Graphics::DESCLAYOUT_ID_VIEW_GLOBAL;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_POSONLY_VBS;
    descLayouts[2] = Graphics::DESCLAYOUT_ID_VIRTUAL_TEXTURE;
    descLayouts[3] = Graphics::DESCLAYOUT_ID_TERRAIN_DATA;
    bOk = LoadShader(shaderFilePaths[6], shaderFilePaths[7], Graphics::SHADER_ID_BASIC_VirtualTexture, windowWidth, windowHeight, Graphics::RENDERPASS_ID_MainView, descLayouts, 4);
    TINKER_ASSERT(bOk);
}

void ShaderManager::CreateAllRenderPasses()
{
    bool bOk = false;
    // Render passes
    // TODO: make the render pass creation stuff more controllable by the app

    // NOTE: this render pass is created inside the vulkan init code
    // color, no depth
    //bOk = Tk::Platform::Graphics::CreateRenderPass(RENDERPASS_ID_SWAP_CHAIN_BLIT, 1, ImageFormat::RGBA8_SRGB, ImageLayout::eUndefined, ImageLayout::ePresent, ImageFormat::Invalid);
    //TINKER_ASSERT(bOk);

    // depth, no color
    bOk = Tk::Platform::Graphics::CreateRenderPass(Graphics::RENDERPASS_ID_ZPrepass, 0, Graphics::ImageFormat::Invalid, Graphics::ImageLayout::eUndefined, Graphics::ImageLayout::eUndefined, Graphics::ImageFormat::Depth_32F);
    TINKER_ASSERT(bOk);

    // color, depth
    bOk = Tk::Platform::Graphics::CreateRenderPass(Graphics::RENDERPASS_ID_MainView, 1, Graphics::ImageFormat::RGBA8_SRGB, Graphics::ImageLayout::eUndefined, Graphics::ImageLayout::eShaderRead, Graphics::ImageFormat::Depth_32F);
    TINKER_ASSERT(bOk);
}

void ShaderManager::LoadAllShaderResources(, uint32 windowWidth, uint32 windowHeight)
{
    bool bOk = false;

    // Descriptor layouts
    Platform::Graphics::DescriptorLayout descriptorLayout = {};

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::Graphics::DescriptorType::eSampledImage;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Platform::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_TEX, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Platform::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Platform::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[2].amount = 1;
    bOk = Tk::Platform::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::Graphics::DescriptorType::eBuffer;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Platform::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_VIEW_GLOBAL, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::Graphics::DescriptorType::eBuffer;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Platform::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_ASSET_INSTANCE, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Platform::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Platform::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[2].amount = 1;
    bOk = Tk::Platform::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_ASSET_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Platform::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_POSONLY_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::Graphics::DescriptorType::eSampledImage;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Platform::Graphics::DescriptorType::eBuffer;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Platform::Graphics::DescriptorType::eSampledImage;
    descriptorLayout.params[2].amount = 1;
    bOk = Tk::Platform::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_VIRTUAL_TEXTURE, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::Graphics::DescriptorType::eBuffer;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Platform::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_TERRAIN_DATA, &descriptorLayout);
    TINKER_ASSERT(bOk);
    //-----

    CreateAllRenderPasses();

    LoadAllShaders(windowWidth, windowHeight);
}

}
}
