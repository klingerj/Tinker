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

void ShaderManager::Startup()
{
    shaderBytecodeAllocator.Init(totalShaderBytecodeMaxSizeInBytes, 1);
}

void ShaderManager::Shutdown()
{
    shaderBytecodeAllocator.ExplicitFree();
}

bool ShaderManager::LoadShader(const Tk::Platform::PlatformAPIFuncs* platformFuncs, const char* vertexShaderFileName, const char* fragmentShaderFileName, Tk::Platform::GraphicsPipelineParams* params)
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

    /*const bool created = platformFuncs->CreateGraphicsPipeline(vertexShaderBuffer, vertexShaderFileSize,
        fragmentShaderBuffer, fragmentShaderFileSize,
        params->blendState, params->depthState,
        params->viewportWidth, params->viewportHeight,
        params->framebufferHandle);
    return created;*/
    return false;
}

void ShaderManager::LoadAllShaders(const Tk::Platform::PlatformAPIFuncs* platformFuncs)
{
    bool bOk = true;

    // Descriptor layouts
    Platform::DescriptorLayout descriptorLayout = {};

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eSampledImage;
    descriptorLayout.params[0].amount = 1;
    platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_SWAP_CHAIN_BLIT_TEX, &descriptorLayout);
    //TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eSSBO;
    descriptorLayout.params[0].amount = 1;
    descriptorLayout.params[1].type = Platform::DescriptorType::eSSBO;
    descriptorLayout.params[1].amount = 1;
    descriptorLayout.params[2].type = Platform::DescriptorType::eSSBO;
    descriptorLayout.params[2].amount = 1;
    platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_SWAP_CHAIN_BLIT_VBS, &descriptorLayout);
    //TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eBuffer;
    descriptorLayout.params[0].amount = 1;
    platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_VIEW_GLOBAL, &descriptorLayout);
    //TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    descriptorLayout.params[0].type = Platform::DescriptorType::eBuffer;
    descriptorLayout.params[0].amount = 1;
    platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_ASSET_INSTANCE, &descriptorLayout);
    //TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    // TODO: populate bindings
    platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_ASSET_VBS, &descriptorLayout);
    //TINKER_ASSERT(bOk);

    descriptorLayout.InitInvalid();
    // TODO: populate bindings
    platformFuncs->CreateDescriptorLayout(DESCLAYOUT_ID_ANIMPOLY_VBS, &descriptorLayout);
    //TINKER_ASSERT(bOk);
    //-----

    // Shaders
    const uint32 numShaderFilepaths = 6;
    const char* shaderFilePaths[numShaderFilepaths] =
    {
        SHADERS_SPV_PATH "blit_vert_glsl.spv",
        SHADERS_SPV_PATH "blit_frag_glsl.spv",
        SHADERS_SPV_PATH "basic_vert_glsl.spv",
        SHADERS_SPV_PATH "basic_frag_glsl.spv",
        SHADERS_SPV_PATH "animpoly_vert_glsl.spv",
        SHADERS_SPV_PATH "animpoly_frag_glsl.spv",
    };

    bool bOk = false;

    // Swap chain blit
    bOk = LoadShader(platformFuncs, shaderFilePaths[0], shaderFilePaths[1], &params);
    TINKER_ASSERT(bOk);

    // ZPrepass
    bOk = LoadShader(platformFuncs, shaderFilePaths[2], nullptr, &params);
    TINKER_ASSERT(bOk);

    // Main view
    bOk = LoadShader(platformFuncs, shaderFilePaths[2], shaderFilePaths[3], &params);
    TINKER_ASSERT(bOk);

    // Animated poly
    bOk = LoadShader(platformFuncs, shaderFilePaths[4], shaderFilePaths[5], &params);
    TINKER_ASSERT(bOk);
}

}
}
