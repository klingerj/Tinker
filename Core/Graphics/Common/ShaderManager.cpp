#include "Graphics/Common/ShaderManager.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "Platform/PlatformGameAPI.h"
#include "Allocators.h"

#ifdef _SHADERS_SPV_DIR
#define SHADERS_SPV_PATH STRINGIFY(_SHADERS_SPV_DIR)
#else
//#define SHADERS_SPV_PATH "..\\Shaders\\spv\\"
#endif

static const uint32 totalShaderBytecodeMaxSizeInBytes = 1024 * 1024 * 100;
static Tk::Core::LinearAllocator g_ShaderBytecodeAllocator;

namespace Tk
{
namespace Core
{
namespace Graphics
{
namespace ShaderManager
{

typedef struct gfx_pipeline_attach_fmts
{
    uint32 numColorRTs = 0;
    uint32 colorRTFormats[MAX_MULTIPLE_RENDERTARGETS] = {};
    uint32 depthFormat = 0;

    void Init()
    {
        numColorRTs = 0;
        for (uint32 i = 0; i < ARRAYCOUNT(colorRTFormats); ++i)
        {
            colorRTFormats[i] = ImageFormat::Invalid;
        }
        depthFormat = ImageFormat::Invalid;
    }
} GraphicsPipelineAttachmentFormats;

static bool LoadShader(const char* vertexShaderFileName, const char* fragmentShaderFileName,
    uint32 shaderID, uint32 viewportWidth, uint32 viewportHeight,
    const GraphicsPipelineAttachmentFormats& pipelineFormats,
    uint32* descLayouts, uint32 numDescLayouts)
{
    uint8* vertexShaderBuffer = nullptr;
    uint8* fragmentShaderBuffer = nullptr;
    uint32 vertexShaderFileSize = 0, fragmentShaderFileSize = 0;

    if (vertexShaderFileName)
    {
        vertexShaderFileSize = Tk::Platform::GetEntireFileSize(vertexShaderFileName);
        vertexShaderBuffer = g_ShaderBytecodeAllocator.Alloc(vertexShaderFileSize, 1);
        TINKER_ASSERT(vertexShaderBuffer);
        Tk::Platform::ReadEntireFile(vertexShaderFileName, vertexShaderFileSize, vertexShaderBuffer);
    }

    if (fragmentShaderFileName)
    {
        fragmentShaderFileSize = Tk::Platform::GetEntireFileSize(fragmentShaderFileName);
        fragmentShaderBuffer = g_ShaderBytecodeAllocator.Alloc(fragmentShaderFileSize, 1);
        TINKER_ASSERT(fragmentShaderBuffer);
        Tk::Platform::ReadEntireFile(fragmentShaderFileName, fragmentShaderFileSize, fragmentShaderBuffer);
    }

    const bool created = Tk::Core::Graphics::CreateGraphicsPipeline(
        vertexShaderBuffer, vertexShaderFileSize,
        fragmentShaderBuffer, fragmentShaderFileSize,
        shaderID, viewportWidth, viewportHeight,
        pipelineFormats.numColorRTs, pipelineFormats.colorRTFormats, pipelineFormats.depthFormat,
        descLayouts, numDescLayouts);
    return created;
}

void Startup()
{
    g_ShaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);
}

void Shutdown()
{
    g_ShaderBytecodeAllocator.ExplicitFree();
}

void ReloadShaders(uint32 newWindowWidth, uint32 newWindowHeight)
{
    Graphics::DestroyAllPSOPerms();
    LoadAllShaders(newWindowWidth, newWindowHeight);
}

void CreateWindowDependentResources(uint32 newWindowWidth, uint32 newWindowHeight)
{
    // TODO: don't reload the shader every time we resize, need to be able to reference existing bytecode... which we do already store
    LoadAllShaders(newWindowWidth, newWindowHeight);
}

void LoadAllShaders(uint32 windowWidth, uint32 windowHeight)
{
    g_ShaderBytecodeAllocator.ExplicitFree();
    g_ShaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);

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

    GraphicsPipelineAttachmentFormats pipelineFormats;

    // Swap chain blit
    descLayouts[0] = Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_TEX;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_VBS;
    pipelineFormats.Init();
    pipelineFormats.numColorRTs = 1;
    pipelineFormats.colorRTFormats[0] = ImageFormat::TheSwapChainFormat;
    bOk = LoadShader(shaderFilePaths[0], shaderFilePaths[1], Graphics::SHADER_ID_SWAP_CHAIN_BLIT, windowWidth, windowHeight, pipelineFormats, descLayouts, 2);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;

    // ZPrepass
    descLayouts[0] = Graphics::DESCLAYOUT_ID_VIEW_GLOBAL;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_ASSET_INSTANCE;
    descLayouts[2] = Graphics::DESCLAYOUT_ID_ASSET_VBS;
    pipelineFormats.Init();
    pipelineFormats.depthFormat = ImageFormat::Depth_32F;
    bOk = LoadShader(shaderFilePaths[2], nullptr, Graphics::SHADER_ID_BASIC_ZPrepass, windowWidth, windowHeight, pipelineFormats, descLayouts, 3);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;

    // Main view
    descLayouts[0] = Graphics::DESCLAYOUT_ID_VIEW_GLOBAL;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_ASSET_INSTANCE;
    descLayouts[2] = Graphics::DESCLAYOUT_ID_ASSET_VBS;
    pipelineFormats.Init();
    pipelineFormats.numColorRTs = 1;
    pipelineFormats.colorRTFormats[0] = ImageFormat::RGBA8_SRGB;
    pipelineFormats.depthFormat = ImageFormat::Depth_32F;
    bOk = LoadShader(shaderFilePaths[2], shaderFilePaths[3], Graphics::SHADER_ID_BASIC_MainView, windowWidth, windowHeight, pipelineFormats, descLayouts, 3);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;

    // Animated poly
    descLayouts[0] = Graphics::DESCLAYOUT_ID_VIEW_GLOBAL;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_POSONLY_VBS;
    pipelineFormats.Init();
    pipelineFormats.numColorRTs = 1;
    pipelineFormats.colorRTFormats[0] = ImageFormat::RGBA8_SRGB;
    pipelineFormats.depthFormat = ImageFormat::Depth_32F;
    bOk = LoadShader(shaderFilePaths[4], shaderFilePaths[5], Graphics::SHADER_ID_ANIMATEDPOLY_MainView, windowWidth, windowHeight, pipelineFormats, descLayouts, 2);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;

    // Virtual texture, basic shader
    /*descLayouts[0] = Graphics::DESCLAYOUT_ID_VIEW_GLOBAL;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_POSONLY_VBS;
    descLayouts[2] = Graphics::DESCLAYOUT_ID_VIRTUAL_TEXTURE;
    descLayouts[3] = Graphics::DESCLAYOUT_ID_TERRAIN_DATA;
    bOk = LoadShader(shaderFilePaths[6], shaderFilePaths[7], Graphics::SHADER_ID_BASIC_VirtualTexture, windowWidth, windowHeight, pipelineFormats, descLayouts, 4);
    TINKER_ASSERT(bOk);*/
}

void LoadAllShaderResources(uint32 windowWidth, uint32 windowHeight)
{
    bool bOk = false;

    // Descriptor layouts
    Tk::Core::Graphics::DescriptorLayout descriptorLayout = {};

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Core::Graphics::DescriptorType::eSampledImage;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Core::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_TEX, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Core::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Tk::Core::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Tk::Core::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[2].amount = 1;
    bOk = Tk::Core::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_SWAP_CHAIN_BLIT_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Core::Graphics::DescriptorType::eBuffer;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Core::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_VIEW_GLOBAL, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Core::Graphics::DescriptorType::eBuffer;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Core::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_ASSET_INSTANCE, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Core::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Tk::Core::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Tk::Core::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[2].amount = 1;
    bOk = Tk::Core::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_ASSET_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Core::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Core::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_POSONLY_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Core::Graphics::DescriptorType::eSampledImage;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Tk::Core::Graphics::DescriptorType::eBuffer;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Tk::Core::Graphics::DescriptorType::eSampledImage;
    descriptorLayout.params[2].amount = 1;
    bOk = Tk::Core::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_VIRTUAL_TEXTURE, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Core::Graphics::DescriptorType::eBuffer;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Core::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_TERRAIN_DATA, &descriptorLayout);
    TINKER_ASSERT(bOk);

    LoadAllShaders(windowWidth, windowHeight);
}

}
}
}
}
