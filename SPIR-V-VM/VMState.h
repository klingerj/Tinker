#ifndef VM_STATE_H
#define VM_STATE_H

#include "CoreDefines.h"
#include "VMTypes.h"
#include "VMContext.h"

struct vm_shader;
typedef struct vm_shader VM_Shader;

#define MAX_FUNCTION_STACK_DEPTH 64
#define MAX_INPUT_DATA_LOCATIONS 64
#define MAX_OUTPUT_DATA_LOCATIONS 8

typedef struct vm_state
{
    // Shader this state was created from
    VM_Shader* ownerShader;

    // Function data
    union function_data
    {
        // Used during shader execution
        struct function_stack
        {
            uint32* funcInsnPtrs[MAX_FUNCTION_STACK_DEPTH];
            uint8 currStackIndex;
        } functionStack;

        // Used while setting up state
        Result* currFuncSetup;
    } functionData;

    // Result ID data
    Result* results;
    uint32 numResultIDs;
    
    // Input data (e.g. vertex buffers, interpolated fragment data)
    void* inputData[MAX_INPUT_DATA_LOCATIONS];
    void* outputData[MAX_OUTPUT_DATA_LOCATIONS];

} VM_State;
VM_State* CreateState_Internal(VM_Context* context, VM_Shader* shader);
void DestroyState_Internal(VM_Context* context, VM_State* state);

ResultID FindEntryPoint(VM_State* state, const char* name);
void CallFunction(VM_Context* context, VM_State* state, ResultID functionID);

void AddStateInputData_Internal(VM_Context* context, VM_State* state, uint32 location, void* data, uint32 dataSizeInBytes);

#endif
