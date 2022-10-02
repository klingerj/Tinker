#include <atlbase.h>
#include <inc/dxcapi.h>

#include <stdio.h>

#include "ShaderCompiler.h"
#include "DataStructures/Vector.h"
#include "Platform/PlatformGameAPI.h"

#ifdef _SHADERS_SRC_DIR
#define SHADERS_SRC_DIR STRINGIFY(_SHADERS_SRC_DIR)
#endif

#ifdef _SHADERS_SPV_DIR
#define SHADERS_SPV_DIR STRINGIFY(_SHADERS_SPV_DIR)
#endif

namespace ShaderType
{

enum : uint32
{
    Vertex = 0,
    Pixel,
    Compute,
    Max
};
}

static const char* ShaderFileSuffixRegexs[ShaderType::Max] =
{
    SHADERS_SRC_DIR "*_VS.hlsl",
    SHADERS_SRC_DIR "*_PS.hlsl",
    SHADERS_SRC_DIR "*_CS.hlsl",
};

// TODO: shader #defines

#define ENTRY_POINT_NAME_WCHAR L"main"

static const wchar_t* CompileFlags_Common[] =
{
    L"DXC_ARG_WARNINGS_ARE_ERRORS",
    L"DXC_ARG_DEBUG",
    L"DXC_ARG_PACK_MATRIX_COLUMN_MAJOR",
    L"-E "
    ENTRY_POINT_NAME_WCHAR,
};

static const wchar_t* CompileFlags_VkSpecific[] =
{
    L"-spirv",
    L"-fspv-target-env=vulkan1.3",
};

static const wchar_t* CompileFlags_Debug[] =
{
    L"-Zi",
    //L"-Qstrip_debug",
    L"-Zss",
};

static const wchar_t* CompileFlags_ShaderSpecific[ShaderType::Max] =
{
    L"-T vs_6_7",
    L"-T ps_6_7",
    L"-T cs_6_7",
};

static void AppendFlags_(const wchar_t** flagsToAppend, uint32 numArgsToAppend, Tk::Core::Vector<const wchar_t*>& argsOut)
{
    for (uint32 i = 0; i < numArgsToAppend; ++i)
    {
        argsOut.PushBackRaw(flagsToAppend[i]);
    }
}
#define AppendFlags(flags, args) AppendFlags_(flags, ARRAYCOUNT(flags), args);
#define AppendSingleFlag(flag, args) AppendFlags_(&flag, 1, args);

static uint32 CompileFile(CComPtr<IDxcCompiler3> pCompiler, CComPtr<IDxcUtils> pUtils, CComPtr<IDxcIncludeHandler> pIncludeHandler, const wchar_t* args, uint32 numArgs, const wchar_t* shaderFilepath, const wchar_t* shaderFilenameWithExt)
{
    CComPtr<IDxcBlobEncoding> pSource = nullptr;
    pUtils->LoadFile((LPCWSTR)shaderFilepath, nullptr, &pSource);
    DxcBuffer Source;
    Source.Ptr = pSource->GetBufferPointer();
    Source.Size = pSource->GetBufferSize();
    Source.Encoding = DXC_CP_ACP;

    CComPtr<IDxcResult> pResults;
    HRESULT compileStatus = pCompiler->Compile(&Source, (LPCWSTR*)args, numArgs, pIncludeHandler, IID_PPV_ARGS(&pResults));
    if (FAILED(compileStatus))
    {
        // Something bad happened inside DXC
        TINKER_ASSERT(0);
        return ShaderCompileErrCode::HasErrors;
    }

    uint32 errCode = ShaderCompileErrCode::Max;

    CComPtr<IDxcBlobUtf8> pErrors = nullptr;
    pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);

    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
    {
        printf("Warnings and Errors:\n%s\n", pErrors->GetStringPointer());
        return ShaderCompileErrCode::HasErrors;
    }
    
    // Compilation succeeded
    errCode = ShaderCompileErrCode::Success;
    // TODO: figure out how to determine if the shader has only warnings

    // Get shader hash
    CComPtr<IDxcBlob> pHash = nullptr;
    if (SUCCEEDED(pResults->GetOutput(DXC_OUT_SHADER_HASH, IID_PPV_ARGS(&pHash), nullptr)) && pHash != nullptr)
    {
        DxcShaderHash* pHashBuf = (DxcShaderHash*)pHash->GetBufferPointer();
        // TODO: do something with it
    }

    CComPtr<IDxcBlob> pShader = nullptr;
    if (SUCCEEDED(pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), nullptr)) && pShader != nullptr)
    {
        // TODO: alloc into linear allocator, memcpy in the bytecode so it can be referenced later, e.g. in hashmap

        // For now, just write out the spv file
        char shaderFilepathSpv[2048] = {};
        uint32 filepathMax = ARRAYCOUNT(shaderFilepathSpv);
        uint32 basepathLen = (uint32)strlen(SHADERS_SPV_DIR);
        memcpy(&shaderFilepathSpv[0], SHADERS_SPV_DIR, basepathLen);
        char* shaderFilenameSpvStart = &shaderFilepathSpv[basepathLen];

        // Remove .hlsl ext, replace with .spv
        const char* hlslExt = "hlsl";
        const uint32 hlslExtLen = 4;
        uint32 shaderFilenameNoExtLen = (uint32)wcslen(shaderFilenameWithExt) - hlslExtLen;
        uint32 numCharsRemainingToWrite = filepathMax - shaderFilenameNoExtLen - 4; // spv + null term
        size_t numCharsWritten = 0;
        wcstombs_s(&numCharsWritten, shaderFilenameSpvStart, filepathMax - basepathLen, shaderFilenameWithExt, shaderFilenameNoExtLen);
        // TODO: better error handling here, but will replace with a better string system
        memcpy(shaderFilenameSpvStart + shaderFilenameNoExtLen, "spv", strlen("spv"));
        *(shaderFilenameSpvStart + shaderFilenameNoExtLen + strlen("spv")) = '\0';
        Tk::Platform::WriteEntireFile(shaderFilepathSpv, (uint32)pShader->GetBufferSize(), (uint8*)pShader->GetBufferPointer());
    }

    return errCode;
}

uint32 CompileAllShadersDX()
{
    //args.PushBackRaw(L"-Qstrip_reflect");
    printf("Compiling all shaders DX!\n");
    return 1;
}

uint32 CompileAllShadersVK()
{
    printf("Compiling all shaders VK!\n\n");

    Tk::Core::Vector<const wchar_t*> args;
    args.Reserve(32);

    HRESULT result;
    CComPtr<IDxcUtils> pUtils;
    CComPtr<IDxcCompiler3> pCompiler;
    result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
    result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));
    // TODO: error handling for these hresults
    
    CComPtr<IDxcIncludeHandler> pIncludeHandler;
    pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);

    // Start compilation
    uint32 allFilesCompiledCleanly = 1;

    // NOTE: only need this due to char -> w_char games
    wchar_t currShaderFilepath[2048] = {};
    uint32 filenameMax = ARRAYCOUNT(currShaderFilepath);
    size_t numCharsWritten = 0;
    mbstowcs_s(&numCharsWritten, currShaderFilepath, ARRAYCOUNT(currShaderFilepath), SHADERS_SRC_DIR, strlen(SHADERS_SRC_DIR));
    --numCharsWritten; // We are going to overwrite the null terminator
    uint32 numCharsRemaining = filenameMax - (uint32)numCharsWritten;
    wchar_t* shaderFilenameStart = &currShaderFilepath[numCharsWritten];

    for (uint32 uiShaderType = 0; uiShaderType < ShaderType::Max; ++uiShaderType)
    {
        args.Clear();
        AppendFlags(CompileFlags_Common, args);
        AppendFlags(CompileFlags_VkSpecific, args);
        AppendSingleFlag(CompileFlags_ShaderSpecific[uiShaderType], args);

        Tk::Platform::FileHandle findFileHandle = Tk::Platform::FindFileOpen(ShaderFileSuffixRegexs[uiShaderType], shaderFilenameStart, numCharsRemaining);
        uint32 findFileError = findFileHandle.h == findFileHandle.eInvalidValue;
        while (!findFileError)
        {
            printf("\nCompiling: %ls...\n", shaderFilenameStart);

            uint32 compileError = CompileFile(pCompiler, pUtils, pIncludeHandler, (const wchar_t*)args.Data(), args.Size(), currShaderFilepath, shaderFilenameStart);
            if (compileError != ShaderCompileErrCode::Success || compileError != ShaderCompileErrCode::HasWarnings)
            {
                allFilesCompiledCleanly = 0;
                // TODO: error reporting?
            }
            else
            {
                // TODO: add entry to hashmap for material library
            }

            printf("Done.\n\n");

            // Reset shader name but keep base path
            memset((uint8*)shaderFilenameStart, 0, numCharsRemaining * sizeof(wchar_t));
            findFileError = Tk::Platform::FindFileNext(findFileHandle, shaderFilenameStart, numCharsRemaining);
        }
        Tk::Platform::FindFileClose(findFileHandle);
    }

    return allFilesCompiledCleanly;
}

int main()
{
    // TODO: parse cmd line args
    bool bVulkan = true;
    uint32 result = 0;
    if (bVulkan)
        result = CompileAllShadersVK();
    else
        result = CompileAllShadersDX();

    return result;
}
