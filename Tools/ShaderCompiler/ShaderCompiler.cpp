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

static uint32 CompileFile(CComPtr<IDxcCompiler3> pCompiler, CComPtr<IDxcUtils> pUtils, CComPtr<IDxcIncludeHandler> pIncludeHandler, LPCWSTR* args, uint32 numArgs, const wchar_t* shaderFilepath, const wchar_t* shaderFilenameWithExt)
{
    CComPtr<IDxcBlobEncoding> pSource = nullptr;
    pUtils->LoadFile((LPCWSTR)shaderFilepath, nullptr, &pSource);
    DxcBuffer Source;
    Source.Ptr = pSource->GetBufferPointer();
    Source.Size = pSource->GetBufferSize();
    Source.Encoding = DXC_CP_ACP;

    CComPtr<IDxcResult> pResults;
    HRESULT compileStatus = pCompiler->Compile(&Source, args, numArgs, pIncludeHandler, IID_PPV_ARGS(&pResults));
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
    //CComPtr<IDxcBlobUtf16> pShaderName = nullptr;
    if (SUCCEEDED(pResults->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&pShader), 0/*&pShaderName*/)) && pShader != nullptr)
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

    Tk::Core::Vector<LPCWSTR> args;
    args.Reserve(16);

    args.PushBackRaw(L"-E");
    args.PushBackRaw(L"main");

    args.PushBackRaw(L"-T");
    args.PushBackRaw(L"vs_6_7");

    //args.PushBackRaw(L"-Zss");
    args.PushBackRaw(L"-Zi");

    //args.PushBackRaw("-Qstrip_debug"); // keep this if we want debug shaders i think

    args.PushBackRaw(L"DXC_ARG_WARNINGS_ARE_ERRORS");
    args.PushBackRaw(L"DXC_ARG_DEBUG");
    args.PushBackRaw(L"DXC_ARG_PACK_MATRIX_COLUMN_MAJOR");

    // Vulkan specific args
    args.PushBackRaw(L"-spirv");
    //args.PushBackRaw(L"-fvk-invert-y");
    args.PushBackRaw(L"-fspv-target-env=vulkan1.3");

    // TODO: shader #defines

    HRESULT result;
    CComPtr<IDxcUtils> pUtils;
    CComPtr<IDxcCompiler3> pCompiler;
    result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
    result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));
    // TODO: check these hresults?

    CComPtr<IDxcIncludeHandler> pIncludeHandler;
    pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);

    // NOTE: only need this due to char -> w_char games
    wchar_t currShaderFilepath[2048] = {};
    uint32 filenameMax = ARRAYCOUNT(currShaderFilepath);
    size_t numCharsWritten = 0;
    mbstowcs_s(&numCharsWritten, currShaderFilepath, ARRAYCOUNT(currShaderFilepath), SHADERS_SRC_DIR, strlen(SHADERS_SRC_DIR));
    --numCharsWritten; // We are going to overwrite the null terminator
    uint32 numCharsRemaining = filenameMax - (uint32)numCharsWritten;
    wchar_t* shaderFilenameStart = &currShaderFilepath[numCharsWritten];

    uint32 allFilesCompiledCleanly = 1;

    Tk::Platform::FileHandle findFileHandle = Tk::Platform::FindFileOpen(SHADERS_SRC_DIR "*vert_hlsl.hlsl", shaderFilenameStart, numCharsRemaining);
    uint32 findFileError = findFileHandle.h == findFileHandle.eInvalidValue;
    while (!findFileError)
    {
        printf("\nCompiling: %ls...\n", shaderFilenameStart);

        uint32 compileError = CompileFile(pCompiler, pUtils, pIncludeHandler, (LPCWSTR*)args.Data(), args.Size(), currShaderFilepath, shaderFilenameStart);
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

    args.Clear();
    args.PushBackRaw(L"-E");
    args.PushBackRaw(L"main");

    args.PushBackRaw(L"-T");
    args.PushBackRaw(L"ps_6_7");

    //args.PushBackRaw(L"-Zss");
    args.PushBackRaw(L"-Zi");

    //args.PushBackRaw("-Qstrip_debug"); // keep this if we want debug shaders i think

    args.PushBackRaw(L"DXC_ARG_WARNINGS_ARE_ERRORS");
    args.PushBackRaw(L"DXC_ARG_DEBUG");
    args.PushBackRaw(L"DXC_ARG_PACK_MATRIX_COLUMN_MAJOR");

    // Vulkan specific args
    args.PushBackRaw(L"-spirv");
    //args.PushBackRaw(L"-fvk-invert-y");
    args.PushBackRaw(L"-fspv-target-env=vulkan1.3");

    findFileHandle = Tk::Platform::FindFileOpen(SHADERS_SRC_DIR "*frag_hlsl.hlsl", shaderFilenameStart, numCharsRemaining);
    findFileError = findFileHandle.h == findFileHandle.eInvalidValue;
    while (!findFileError)
    {
        printf("\nCompiling: %ls...\n", shaderFilenameStart);

        uint32 compileError = CompileFile(pCompiler, pUtils, pIncludeHandler, (LPCWSTR*)args.Data(), args.Size(), currShaderFilepath, shaderFilenameStart);
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
