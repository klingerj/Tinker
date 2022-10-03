#include <atlbase.h>
#include <inc/dxcapi.h>

#include <stdio.h>

#include "ShaderCompiler.h"
#include "DataStructures/Vector.h"
#include "Platform/PlatformGameAPI.h"

static CComPtr<IDxcUtils> g_pUtils;
static CComPtr<IDxcCompiler3> g_pCompiler;
static CComPtr<IDxcIncludeHandler> g_pIncludeHandler;

Tk::Core::Vector<const wchar_t*> g_args;

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
    L"-E ",
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

static const wchar_t* CompileFlags_ShaderSpecific[ShaderType::Max][1] =
{
    { L"-T vs_6_7" },
    { L"-T ps_6_7" },
    { L"-T cs_6_7" },
};

static void AppendArgs_(const wchar_t** argsToAppend, uint32 numArgsToAppend, Tk::Core::Vector<const wchar_t*>& argsOut)
{
    for (uint32 i = 0; i < numArgsToAppend; ++i)
    {
        argsOut.PushBackRaw(argsToAppend[i]);
    }
}
#define AppendArgsToList(args, argsList) AppendArgs_(args, ARRAYCOUNT(args), argsList);

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

    uint32 errCode = ShaderCompileErrCode::HasErrors;

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

static uint32 InitCompiler()
{
    return !(FAILED(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&g_pUtils))) ||
           FAILED(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&g_pCompiler))) ||
           FAILED(g_pUtils->CreateDefaultIncludeHandler(&g_pIncludeHandler)));
}

uint32 CompileAllShadersDX()
{
    printf("DX codepath not implemented yet :)");
    return ShaderCompileErrCode::NonShaderError;
}

uint32 CompileAllShadersVK()
{
    if (!InitCompiler())
        return ShaderCompileErrCode::NonShaderError;
    
    // Start compilation
    uint32 errorCode = ShaderCompileErrCode::Success;

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
        g_args.Clear();
        AppendArgsToList(CompileFlags_Common, g_args);
        AppendArgsToList(CompileFlags_VkSpecific, g_args);
        AppendArgsToList(CompileFlags_ShaderSpecific[uiShaderType], g_args);

        Tk::Platform::FileHandle findFileHandle = Tk::Platform::FindFileOpen(ShaderFileSuffixRegexs[uiShaderType], shaderFilenameStart, numCharsRemaining);
        uint32 findFileError = findFileHandle.h == findFileHandle.eInvalidValue;
        while (!findFileError)
        {
            printf("\nCompiling: %ls...\n", shaderFilenameStart);

            uint32 compileError = CompileFile(g_pCompiler, g_pUtils, g_pIncludeHandler, (const wchar_t*)g_args.Data(), g_args.Size(), currShaderFilepath, shaderFilenameStart);
            if (compileError == ShaderCompileErrCode::Success || compileError == ShaderCompileErrCode::HasWarnings)
            {
                // TODO: add entry to hashmap for material library, get the bytecode from the compile call
            }
            else
            {
                errorCode = compileError;
            }

            printf("Done.\n\n");

            // Reset shader name but keep base path
            memset((uint8*)shaderFilenameStart, 0, numCharsRemaining * sizeof(wchar_t));
            findFileError = Tk::Platform::FindFileNext(findFileHandle, shaderFilenameStart, numCharsRemaining);
        }
        Tk::Platform::FindFileClose(findFileHandle);
    }

    return errorCode;
}

int main(int argc, char* argv[])
{
    uint32 bVulkan = false;
    if (argc != 2)
    {
        printf("Invalid number of arguments provided. Usage: TinkerSC.exe [VK | DX]\n");
        return ShaderCompileErrCode::NonShaderError;
    }
    else
    {
        if (strncmp(argv[1], "VK", strlen("VK")) == 0)
        {
            bVulkan = 1u;
        }
        else if (strncmp(argv[1], "DX", strlen("DX")) == 0)
        {
            bVulkan = 0u;
        }
        else
        {
            printf("Unrecognized argument provided. Usage: TinkerSC.exe [VK | DX]\n");
            return ShaderCompileErrCode::NonShaderError;
        }
    }

    g_args.Reserve(32);
    uint32 result = ShaderCompileErrCode::NonShaderError;
    if (bVulkan)
        result = CompileAllShadersVK();
    else
        result = CompileAllShadersDX();

    switch (result)
    {
        case ShaderCompileErrCode::Success:
        case ShaderCompileErrCode::HasWarnings:
        {
            printf("Compilation succeeded.\n");
            break;
        }

        case ShaderCompileErrCode::HasErrors:
        {
            printf("Compilation failed with shader errors.\n");
            break;
        }

        default:
        {
            printf("Compilation failed due to some error.\n");
            break;
        }
    }

    return result;
}
