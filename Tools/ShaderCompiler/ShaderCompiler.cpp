#include <atlbase.h>
#include <inc/dxcapi.h>

#include <stdio.h>

#include "ShaderCompiler.h"
#include "DataStructures/Vector.h"
#include "Platform/PlatformGameAPI.h"

#ifdef _SHADERS_SRC_DIR
#define SHADERS_SRC_DIR STRINGIFY(_SHADERS_SRC_DIR)
#endif

static uint32 CompileFile(CComPtr<IDxcCompiler3> pCompiler, CComPtr<IDxcUtils> pUtils, CComPtr<IDxcIncludeHandler> pIncludeHandler, LPCWSTR* args, uint32 numArgs, const wchar_t* shaderFilename)
{
    CComPtr<IDxcBlobEncoding> pSource = nullptr;
    pUtils->LoadFile((LPCWSTR)shaderFilename, nullptr, &pSource);
    DxcBuffer Source;
    Source.Ptr = pSource->GetBufferPointer();
    Source.Size = pSource->GetBufferSize();
    Source.Encoding = DXC_CP_ACP;

    CComPtr<IDxcResult> pResults;
    HRESULT compileResult = pCompiler->Compile(&Source, args, numArgs, pIncludeHandler, IID_PPV_ARGS(&pResults));
    if (compileResult != S_OK)
    {
        // Something bad happened
        TINKER_ASSERT(0);
        // TODO: error report better here
    }

    // TODO: figure out how to differentiate errors from warnings
    /*if (pResults->HasOutput())
    {
        // TODO: can check if there are errors
    }*/

    CComPtr<IDxcBlobUtf8> pErrors = nullptr;
    pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);

    // IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.
    if (pErrors && pErrors->GetStringLength() != 0)
    {
        wprintf(L"Warnings and Errors:\n%S\n", pErrors->GetStringPointer());
    }

    // Determine error code
    // TODO: make sure:
    /*
    success means hresult == s_ok and no error buffer
    warnings means hresult == s_ok and yes error buffer
    errors means hresult != s_ok and yes error buffer
    */
    uint32 errCode = ShaderCompileErrCode::Max;

    if (pErrors && pErrors->GetStringLength() != 0)
        errCode = ShaderCompileErrCode::HasErrors;
    else
        errCode = ShaderCompileErrCode::Success;

    /*if (compileResult != S_OK)
    {
        TINKER_ASSERT(pErrors && pErrors->GetStringLength() != 0);
        if (pErrors && pErrors->GetStringLength() != 0)
            errCode = ShaderCompileErrCode::HasErrors;
        else
        {
            TINKER_ASSERT(0);
            printf("Shader Compiler - HResult was != S_OK but no pErrors buffer from DXC.\n");
        }
    }
    else
    {
        if (pErrors && pErrors->GetStringLength() != 0)
            errCode = ShaderCompileErrCode::HasWarnings;
        else
            errCode = ShaderCompileErrCode::Success;
    }*/
    return errCode;
}

uint32 CompileAllShadersDX()
{
    //args.PushBackRaw(L"-Qstrip_reflect");
    printf("Compiling all shaders DX!\n");
    return 1;
}

//#include <locale.h>
uint32 CompileAllShadersVK()
{
    printf("Compiling all shaders VK!\n");

    Tk::Core::Vector<LPCWSTR> args; // TODO: make this less bad later
    args.PushBackRaw(L"-E");
    args.PushBackRaw(L"main");

    args.PushBackRaw(L"-T");
    args.PushBackRaw(L"vs_6_7"); // TODO: VS, PS, CS

    //args.PushBackRaw("-Qstrip_debug"); // keep this if we want debug shaders i think

    args.PushBackRaw(L"DXC_ARG_WARNINGS_ARE_ERRORS");
    args.PushBackRaw(L"DXC_ARG_DEBUG");
    args.PushBackRaw(L"DXC_ARG_PACK_MATRIX_COLUMN_MAJOR");

    // Vulkan specific args
    args.PushBackRaw(L"-spirv");
    args.PushBackRaw(L"-fvk-invert-y");
    // TODO: we definitely want more here eventually

    // TODO: shader #defines

    HRESULT result;

    CComPtr<IDxcUtils> pUtils;
    CComPtr<IDxcCompiler3> pCompiler;
    result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
    result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));

    CComPtr<IDxcIncludeHandler> pIncludeHandler;
    pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);

    // NOTE: only need this due to char -> w_char games
    wchar_t CurrShaderFilename[2048] = {};
    uint32 FilenameMax = ARRAYCOUNT(CurrShaderFilename);
    size_t NumCharsWritten = 0;
    mbstowcs_s(&NumCharsWritten, CurrShaderFilename, ARRAYCOUNT(CurrShaderFilename), SHADERS_SRC_DIR, strlen(SHADERS_SRC_DIR));
    --NumCharsWritten; // We are going to overwrite the null terminator
    uint32 CharsRemaining = FilenameMax - (uint32)NumCharsWritten;
    wchar_t* ShaderFilenameStart = &CurrShaderFilename[NumCharsWritten];

    Tk::Platform::FileHandle FindFileHandle = Tk::Platform::FindFileOpen(SHADERS_SRC_DIR "*.hlsl", ShaderFilenameStart, CharsRemaining);
    //setlocale(LC_ALL, "");
    //printf("%ls\n", CurrShaderFilename);

    uint32 FindFileError = FindFileHandle.h == FindFileHandle.eInvalidValue;
    uint32 AllFilesCompiledCleanly = 1;
    while (!FindFileError)
    {
        uint32 CompileError = CompileFile(pCompiler, pUtils, pIncludeHandler, (LPCWSTR*)args.Data(), args.Size(), CurrShaderFilename);
        if (CompileError != ShaderCompileErrCode::Success || CompileError != ShaderCompileErrCode::HasWarnings)
        {
            AllFilesCompiledCleanly = 0;
            // TODO: error reporting
        }
        else
        {
            // TODO: for now this will just output the spv file, otherwise this would add a material library entry for the library file
            char CurrShaderFilenameSPV[2048] = {};
            //Tk::Platform::WriteEntireFile(filename, filesizeInBytes, bytecode*);
        }

        memset(CurrShaderFilename, 0, ARRAYCOUNT(CurrShaderFilename) * sizeof(uint16));
        FindFileError = Tk::Platform::FindFileNext(FindFileHandle, &CurrShaderFilename[0], CharsRemaining);
    }
    Tk::Platform::FindFileClose(FindFileHandle);

    return 0;
}

int main()
{
    // TODO: parse args
    bool bVulkan = true;
    uint32 result = 0;
    if (bVulkan)
        result = CompileAllShadersVK();
    else
        result = CompileAllShadersDX();

    return result;
}
