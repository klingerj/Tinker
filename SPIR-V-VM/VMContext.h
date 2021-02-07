#ifndef VM_CONTEXT_H
#define VM_CONTEXT_H

#include "KHR/spirv.h"
#include "CoreDefines.h"
#include "SpirvOps.h"

#include <cstdlib>

#define OPCODE_MAX 256 // highest opcode supported

struct VM_Context
{
    op_handler* opHandlers[OPCODE_MAX];
};
inline VM_Context* CreateContext_Internal()
{
    VM_Context* context = (VM_Context*)malloc(sizeof(VM_Context));
    for (uint32 i = 0; i < OPCODE_MAX; ++i)
    {
        context->opHandlers[i] = OpHandler_Stub;
    }

    // Add supported opcode handlers
    context->opHandlers[SpvOpSource] = OpHandler_OpSource;
    context->opHandlers[SpvOpName] = OpHandler_OpName;
    context->opHandlers[SpvOpExtInstImport] = OpHandler_OpExtInstImport;
    context->opHandlers[SpvOpMemoryModel] = OpHandler_OpMemoryModel;
    context->opHandlers[SpvOpEntryPoint] = OpHandler_OpEntryPoint;
    context->opHandlers[SpvOpExecutionMode] = OpHandler_OpExecutionMode;
    context->opHandlers[SpvOpCapability] = OpHandler_OpCapability;
    context->opHandlers[SpvOpTypeVoid] = OpHandler_OpTypeVoid;
    context->opHandlers[SpvOpTypeFloat] = OpHandler_OpTypeFloat;
    context->opHandlers[SpvOpTypeVector] = OpHandler_OpTypeVector;
    context->opHandlers[SpvOpTypePointer] = OpHandler_OpTypePointer;
    context->opHandlers[SpvOpTypeFunction] = OpHandler_OpTypeFunction;
    context->opHandlers[SpvOpConstant] = OpHandler_OpConstant;
    context->opHandlers[SpvOpConstantComposite] = OpHandler_OpConstantComposite;
    context->opHandlers[SpvOpFunction] = OpHandler_OpFunction;
    context->opHandlers[SpvOpFunctionEnd] = OpHandler_OpFunctionEnd;
    context->opHandlers[SpvOpVariable] = OpHandler_OpVariable;
    context->opHandlers[SpvOpLoad] = OpHandler_OpLoad;
    context->opHandlers[SpvOpDecorate] = OpHandler_OpDecorate;
    context->opHandlers[SpvOpLabel] = OpHandler_OpLabel;
    context->opHandlers[SpvOpReturn] = OpHandler_OpReturn;

    return context;
}

inline void DestroyContext_Internal(VM_Context* context)
{
    free(context);
}

#endif
