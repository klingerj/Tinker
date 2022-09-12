#include <atlbase.h>
#include <inc/dxcapi.h>

#include <stdio.h>

#include "ShaderCompiler.h"
#include "DataStructures/Vector.h"
#include "Platform/PlatformGameAPI.h"

#ifdef _SHADERS_SRC_DIR
#define SHADERS_SRC_DIR STRINGIFY(_SHADERS_SRC_DIR)
#endif

static uint32 CompileFile(CComPtr<IDxcCompiler3> pCompiler, CComPtr<IDxcUtils> pUtils, CComPtr<IDxcIncludeHandler> pIncludeHandler, const Tk::Core::Vector<LPCWSTR>& args, const wchar_t* shaderFilename)
{
    CComPtr<IDxcBlobEncoding> pSource = nullptr;
    pUtils->LoadFile((LPCWSTR)shaderFilename, nullptr, &pSource);
    DxcBuffer Source;
    Source.Ptr = pSource->GetBufferPointer();
    Source.Size = pSource->GetBufferSize();
    Source.Encoding = DXC_CP_ACP;

    CComPtr<IDxcResult> pResults;
    pCompiler->Compile(&Source, (LPCWSTR*)args.Data(), args.Size(), pIncludeHandler, IID_PPV_ARGS(&pResults));

    CComPtr<IDxcBlobUtf8> pErrors = nullptr;
    pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
    // Note that d3dcompiler would return null if no errors or warnings are present.  
    // IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.
    if (pErrors != nullptr && pErrors->GetStringLength() != 0)
        wprintf(L"Warnings and Errors:\n%S\n", pErrors->GetStringPointer());

    return 0;
}

uint32 CompileAllShadersDX()
{
    //args.PushBackRaw(L"-Qstrip_reflect");

    printf("Compiling all shaders DX!\n");
    return 1;
}

#include <locale.h>
uint32 CompileAllShadersVK()
{
    Tk::Core::Vector<uint8> vec;
    vec.Reserve(16u);
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

    // TODO: grab all files in the directory
    wchar_t CurrShaderFilename[2048] = {};
    uint32 FilenameMax = ARRAYCOUNT(CurrShaderFilename);
    size_t NumCharsWritten = 0;
    mbstowcs_s(&NumCharsWritten, CurrShaderFilename, ARRAYCOUNT(CurrShaderFilename), SHADERS_SRC_DIR, strlen(SHADERS_SRC_DIR));
    --NumCharsWritten; // We are going to overwrite the null terminator
    uint32 CharsRemaining = FilenameMax - (uint32)NumCharsWritten;
    wchar_t* ShaderFilenameStart = &CurrShaderFilename[NumCharsWritten];

    Tk::Platform::FileHandle FindFileHandle = Tk::Platform::FindFileOpen(SHADERS_SRC_DIR "*.hlsl", ShaderFilenameStart, CharsRemaining);
    setlocale(LC_ALL, "");
    printf("%ls\n", CurrShaderFilename);

    uint32 FindFileError = FindFileHandle.h == FindFileHandle.eInvalidValue;
    while (!FindFileError)
    {
        uint32 CompileError = CompileFile(pCompiler, pUtils, pIncludeHandler, args, CurrShaderFilename);
        if (CompileError)
        {
            // TODO: report something somewhere
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
