#include "SpirvVM.h"

#include "VMContext.h"
#include "VMState.h"
#include "VMShader.h"

VM_Context* CreateContext()
{
    return CreateContext_Internal();
}
void DestroyContext(VM_Context* context)
{
    DestroyContext_Internal(context);
}
VM_Shader* CreateShader(VM_Context* context, const uint32* spvFile, uint32 fileSizeInBytes)
{
    return CreateShader_Internal(context, spvFile, fileSizeInBytes);
}
void DestroyShader(VM_Context* context, VM_Shader* shader)
{
    DestroyShader_Internal(context, shader);
}
VM_State* CreateState(VM_Context* context, VM_Shader* shader)
{
    return CreateState_Internal(context, shader);
}
void DestroyState(VM_Context* context, VM_State* state)
{
    DestroyState_Internal(context, state);
}

uint8 CallEntryPointByName(VM_Context* context, VM_State* state, const char* name)
{
    ResultID entryPoint = FindEntryPoint(state, name);
    if (entryPoint != (ResultID)-1)
    {
        CallFunction(context, state, entryPoint);
        return 0;
    }
    else
    {
        printf("Unable to find entry point by name: %s\n", name);
        return 1;
    }
}

void AddStateInputData(VM_Context* context, VM_State* state, uint32 location, void* data, uint32 dataSizeInBytes)
{
    AddStateInputData_Internal(context, state, location, data, dataSizeInBytes);
}
