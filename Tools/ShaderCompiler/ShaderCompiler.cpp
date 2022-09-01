#include <atlbase.h>
//#include <windows.h>
#include <inc/dxcapi.h>

#include <stdio.h>

#include "ShaderCompiler.h"
#include "DataStructures/Vector.h"
#include "Platform/PlatformGameAPI.h"

uint32 CompileAllShadersDX()
{
    printf("Compiling all shaders DX!\n");
    return 1;
}

uint32 CompileAllShadersVK()
{
    Tk::Core::Vector<uint8> vec;
    vec.Reserve(16u);
    printf("Compiling all shaders VK!\n");

    Tk::Core::Vector<const char*> args;
    args.PushBackRaw("-E");
    args.PushBackRaw("main");

    args.PushBackRaw("-T");
    args.PushBackRaw("vs_6_7"); // TODO: VS, PS, CS

    //args.PushBackRaw("-Qstrip_debug"); // keep this if we want debug shaders
    args.PushBackRaw("-Qstrip_reflect");

    args.PushBackRaw("DXC_ARG_WARNINGS_ARE_ERRORS");
    args.PushBackRaw("DXC_ARG_DEBUG");
    args.PushBackRaw("DXC_ARG_PACK_MATRIX_COLUMN_MAJOR");

    // TODO: add vulkan args

    // TODO: #defines

    HRESULT result;

    CComPtr<IDxcUtils> pUtils;
    CComPtr<IDxcCompiler3> pCompiler;
    result = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&pUtils));
    result = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&pCompiler));

    CComPtr<IDxcIncludeHandler> pIncludeHandler;
    pUtils->CreateDefaultIncludeHandler(&pIncludeHandler);

    CComPtr<IDxcBlobEncoding> pSource = nullptr;
    pUtils->LoadFile(L"myshader.hlsl", nullptr, &pSource);
    DxcBuffer Source;
    Source.Ptr = pSource->GetBufferPointer();
    Source.Size = pSource->GetBufferSize();
    Source.Encoding = DXC_CP_ACP;

    CComPtr<IDxcResult> pResults;
    pCompiler->Compile(
        &Source,                // Source buffer.
        (LPCWSTR*)args.Data(),                // Array of pointers to arguments.
        args.Size(),      // Number of arguments.
        pIncludeHandler,        // User-provided interface to handle #include directives (optional).
        IID_PPV_ARGS(&pResults) // Compiler output status, buffer, and errors.
    );

    CComPtr<IDxcBlobUtf8> pErrors = nullptr;
    pResults->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrors), nullptr);
    // Note that d3dcompiler would return null if no errors or warnings are present.  
    // IDxcCompiler3::Compile will always return an error buffer, but its length will be zero if there are no warnings or errors.
    //if (pErrors != nullptr && pErrors->GetStringLength() != 0)
        //wprintf(L"Warnings and Errors:\n%S\n", pErrors->GetStringPointer());

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
