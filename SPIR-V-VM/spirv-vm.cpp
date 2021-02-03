#include "spirv-vm.h"

#include <stdio.h>
#include <cstdlib>

int main()
{
    const char* spvFilePath = "../Shaders/spv/basic_frag_glsl.spv";
    printf("SPV file specified: %s\n", spvFilePath);

    FILE* spvFP = fopen(spvFilePath, "rb");
    fseek(spvFP, 0L, SEEK_END);
    uint32 fileSizeInBytes = ftell(spvFP); // get file size
    fseek(spvFP, 0L, SEEK_SET);
    uint8* fileBuffer = (uint8*)malloc(fileSizeInBytes);
    fread(fileBuffer, 1, fileSizeInBytes, spvFP);

    {
        SPIRV_VM::SPIRV_VM vm;
        vm.Init();

        SPIRV_VM::VM_Shader shader = vm.CreateShader((uint32*)fileBuffer, fileSizeInBytes);
        vm.ExecuteShader(shader);

        const SPIRV_VM::SPIRV_VM_State state = vm.QueryState();
        printf("VM state after execution: \n");
        printf("Reg A: %llu\n", state.regA);
        printf("Reg B: %llu\n", state.regB);

        vm.DestroyShader(shader);
    }

    free(fileBuffer);
    fclose(spvFP);

    return 0;
}
