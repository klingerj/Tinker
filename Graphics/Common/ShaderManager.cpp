#include "Graphics/Common/ShaderManager.h"
#include "Graphics/Common/GraphicsCommon.h"
#include "Platform/PlatformGameAPI.h"
#include "Allocators.h"

#ifdef _SHADERS_SPV_DIR
#define SHADERS_SPV_PATH STRINGIFY(_SHADERS_SPV_DIR)
#endif

static const uint32 totalShaderBytecodeMaxSizeInBytes = 1024 * 1024 * 100;
static Tk::Core::LinearAllocator g_ShaderBytecodeAllocator;

namespace Tk
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
    uint32 shaderID, const GraphicsPipelineAttachmentFormats& pipelineFormats,
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

    const bool created = Tk::Graphics::CreateGraphicsPipeline(
        vertexShaderBuffer, vertexShaderFileSize,
        fragmentShaderBuffer, fragmentShaderFileSize,
        shaderID,
        pipelineFormats.numColorRTs, pipelineFormats.colorRTFormats, pipelineFormats.depthFormat,
        descLayouts, numDescLayouts);
    return created;
}

static bool LoadComputeShader(const char* computeShaderFileName,
    uint32 shaderID, uint32* descLayouts, uint32 numDescLayouts)
{
    uint8* computeShaderBuffer = nullptr;
    uint32 computeShaderFileSize = 0;

    if (computeShaderFileName)
    {
        computeShaderFileSize = Tk::Platform::GetEntireFileSize(computeShaderFileName);
        computeShaderBuffer = g_ShaderBytecodeAllocator.Alloc(computeShaderFileSize, 1);
        TINKER_ASSERT(computeShaderBuffer);
        Tk::Platform::ReadEntireFile(computeShaderFileName, computeShaderFileSize, computeShaderBuffer);
    }

    const bool created = Tk::Graphics::CreateComputePipeline(
        computeShaderBuffer, computeShaderFileSize,
        shaderID, descLayouts, numDescLayouts);
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

void ReloadShaders()
{
    Graphics::DestroyAllPSOPerms();
    LoadAllShaders();
}

void LoadAllShaders()
{
    g_ShaderBytecodeAllocator.ExplicitFree();
    g_ShaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);

    bool bOk = false;

    // Shaders
    const uint32 numShaderFilepaths = 12;
    const char* shaderFilePaths[numShaderFilepaths] =
    {
        SHADERS_SPV_PATH "blit_VS.spv",
        SHADERS_SPV_PATH "tonemapping_PS.spv",
        SHADERS_SPV_PATH "basic_VS.spv",
        SHADERS_SPV_PATH "basic_PS.spv",
        SHADERS_SPV_PATH "animpoly_VS.spv",
        SHADERS_SPV_PATH "animpoly_PS.spv",
        SHADERS_SPV_PATH "imgui_VS.spv",
        SHADERS_SPV_PATH "imgui_PS.spv",
        SHADERS_SPV_PATH "grayscale_CS.spv",
        SHADERS_SPV_PATH "blit_clear_VS.spv",
        SHADERS_SPV_PATH "blit_clear_PS.spv",
        SHADERS_SPV_PATH "blit_PS.spv",
    };

    uint32 descLayouts[MAX_DESCRIPTOR_SETS_PER_SHADER] = {};
    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;
    }

    GraphicsPipelineAttachmentFormats pipelineFormats;

    // Quad blit tonemap
    descLayouts[0] = Graphics::DESCLAYOUT_ID_QUAD_BLIT_TEX;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_QUAD_BLIT_VBS;
    pipelineFormats.Init();
    pipelineFormats.numColorRTs = 1;
    pipelineFormats.colorRTFormats[0] = ImageFormat::RGBA16_Float;
    bOk = LoadShader(shaderFilePaths[0], shaderFilePaths[11], Graphics::SHADER_ID_QUAD_BLIT_TONEMAP, pipelineFormats, descLayouts, 2);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;
    }

    // Quad blit copy
    descLayouts[0] = Graphics::DESCLAYOUT_ID_QUAD_BLIT_TEX;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_QUAD_BLIT_VBS;
    pipelineFormats.Init();
    pipelineFormats.numColorRTs = 1;
    pipelineFormats.colorRTFormats[0] = ImageFormat::TheSwapChainFormat;
    bOk = LoadShader(shaderFilePaths[0], shaderFilePaths[11], Graphics::SHADER_ID_QUAD_BLIT_RGBA8, pipelineFormats, descLayouts, 2);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;
    }

    // Quad blit clear 
    descLayouts[0] = Graphics::DESCLAYOUT_ID_QUAD_BLIT_VBS;
    pipelineFormats.Init();
    pipelineFormats.numColorRTs = 1;
    pipelineFormats.colorRTFormats[0] = ImageFormat::TheSwapChainFormat;
    bOk = LoadShader(shaderFilePaths[9], shaderFilePaths[10], Graphics::SHADER_ID_QUAD_BLIT_CLEAR, pipelineFormats, descLayouts, 1);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;
    }

    // Imgui debug ui pass
    descLayouts[0] = Graphics::DESCLAYOUT_ID_IMGUI_TEX;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_IMGUI_VBS;
    pipelineFormats.Init();
    pipelineFormats.numColorRTs = 1;
    pipelineFormats.colorRTFormats[0] = ImageFormat::RGBA16_Float;
    bOk = LoadShader(shaderFilePaths[6], shaderFilePaths[7], Graphics::SHADER_ID_IMGUI_DEBUGUI, pipelineFormats, descLayouts, 2);
    TINKER_ASSERT(bOk);

    // TODO: temporary, need to resolve how to handle RT formats 
    pipelineFormats.colorRTFormats[0] = ImageFormat::TheSwapChainFormat;
    bOk = LoadShader(shaderFilePaths[6], shaderFilePaths[7], Graphics::SHADER_ID_IMGUI_DEBUGUI_RGBA8, pipelineFormats, descLayouts, 2);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;
    }

    // ZPrepass
    descLayouts[0] = Graphics::DESCLAYOUT_ID_BINDLESS_CONSTANTS;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_ASSET_VBS;
    pipelineFormats.Init();
    pipelineFormats.depthFormat = ImageFormat::Depth_32F;
    bOk = LoadShader(shaderFilePaths[2], nullptr, Graphics::SHADER_ID_BASIC_ZPrepass, pipelineFormats, descLayouts, 2);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;
    }

    // Main view
    descLayouts[0] = Graphics::DESCLAYOUT_ID_BINDLESS_CONSTANTS;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_ASSET_VBS;
    descLayouts[2] = Graphics::DESCLAYOUT_ID_BINDLESS_SAMPLED_TEXTURES;
    pipelineFormats.Init();
    pipelineFormats.numColorRTs = 1;
    pipelineFormats.colorRTFormats[0] = ImageFormat::RGBA16_Float;
    pipelineFormats.depthFormat = ImageFormat::Depth_32F;
    bOk = LoadShader(shaderFilePaths[2], shaderFilePaths[3], Graphics::SHADER_ID_BASIC_MainView, pipelineFormats, descLayouts, 3);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;
    }

    // Animated poly
    descLayouts[0] = Graphics::DESCLAYOUT_ID_BINDLESS_CONSTANTS;
    descLayouts[1] = Graphics::DESCLAYOUT_ID_POSONLY_VBS;
    pipelineFormats.Init();
    pipelineFormats.numColorRTs = 1;
    pipelineFormats.colorRTFormats[0] = ImageFormat::RGBA16_Float;
    pipelineFormats.depthFormat = ImageFormat::Depth_32F;
    bOk = LoadShader(shaderFilePaths[4], shaderFilePaths[5], Graphics::SHADER_ID_ANIMATEDPOLY_MainView, pipelineFormats, descLayouts, 2);
    TINKER_ASSERT(bOk);

    for (uint32 i = 0; i < MAX_DESCRIPTOR_SETS_PER_SHADER; ++i)
    {
        descLayouts[i] = Graphics::DESCLAYOUT_ID_MAX;
    }

    /// Compute
    
    // Copy
    descLayouts[0] = Graphics::DESCLAYOUT_ID_COMPUTE_COPY;
    bOk = LoadComputeShader(shaderFilePaths[8], Graphics::SHADER_ID_COMPUTE_COPY, descLayouts, 1);
    TINKER_ASSERT(bOk);
}

void LoadAllShaderResources()
{
    bool bOk = false;

    // Descriptor layouts
    Tk::Graphics::DescriptorLayout descriptorLayout = {};

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_BINDLESS_CONSTANTS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Graphics::DescriptorType::eArrayOfTextures;
    descriptorLayout.params[0].amount = DESCRIPTOR_BINDLESS_ARRAY_LIMIT;
    bOk = Tk::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_BINDLESS_SAMPLED_TEXTURES, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Graphics::DescriptorType::eSampledImage;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_QUAD_BLIT_TEX, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Tk::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Tk::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[2].amount = 1;
    bOk = Tk::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_QUAD_BLIT_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Tk::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Tk::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[2].amount = 1;
    bOk = Tk::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_IMGUI_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Graphics::DescriptorType::eSampledImage;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_IMGUI_TEX, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Tk::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Tk::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[2].amount = 1;
    bOk = Tk::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_ASSET_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Graphics::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    bOk = Tk::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_POSONLY_VBS, &descriptorLayout);
    TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Tk::Graphics::DescriptorType::eStorageImage;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Tk::Graphics::DescriptorType::eStorageImage;
    descriptorLayout.params[1].amount = 1;
    bOk = Tk::Graphics::CreateDescriptorLayout(Graphics::DESCLAYOUT_ID_COMPUTE_COPY, &descriptorLayout);
    TINKER_ASSERT(bOk);

    LoadAllShaders();
}

}
}
}
