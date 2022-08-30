#include <windows.h>
#include "inc/dxcapi.h"

#include <stdio.h>

#include "ShaderCompiler.h"
#include "DataStructures/Vector.h"

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

    // TODO: set compile args, compile each shader
    DxcBuffer sourceBuffer;
    sourceBuffer.Size = 0;

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
