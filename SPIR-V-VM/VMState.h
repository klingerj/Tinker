#ifndef VM_STATE_H
#define VM_STATE_H

#include "KHR/spirv.h"
#include "KHR/GLSL.std.450.h"
#include "CoreDefines.h"
#include "VMTypes.h"
#include "VMContext.h"

struct VM_Shader;
struct VM_State
{
    // Shader this state was created from
    VM_Shader* ownerShader;

    // Result ID data
    uint32 numResultIDs;
    Result* results;
    
    // TODO: store function stack data
};
VM_State* CreateState_Internal(VM_Context* context, VM_Shader* shader);
void DestroyState_Internal(VM_Context* context, VM_State* state);

#endif
