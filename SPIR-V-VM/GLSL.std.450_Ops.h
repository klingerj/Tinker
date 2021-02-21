#ifndef GLSL_STD_450_OPS_H
#define GLSL_STD_450_OPS_H

#include "CoreDefines.h"
#include "VMTypes.h"

struct vm_state;
typedef struct vm_state VM_State;
struct vm_context;
typedef struct vm_context VM_Context;
#define GLSL_STD_450_OP_HANDLER(name) void name(VM_Context* context, VM_State* state, const uint32** insnStreamPtr, uint16 numWordsAfterOpcode, uint32 resultID, uint32 resultTypeID)
typedef GLSL_STD_450_OP_HANDLER(glsl_std_450_op_handler);
inline GLSL_STD_450_OP_HANDLER(Glsl_std_450_OpHandler_Stub)
{
    for (uint16 uiWord = 0; uiWord < numWordsAfterOpcode; ++uiWord)
    {
        CONSUME_SPIRV_WORD(insnStreamPtr);
    }
}

GLSL_STD_450_OP_HANDLER(Glsl_std_450_FClamp); // 43

#endif
