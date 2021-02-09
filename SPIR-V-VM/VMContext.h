#ifndef VM_CONTEXT_H
#define VM_CONTEXT_H

#include "KHR/spirv.h"
#include "CoreDefines.h"
#include "SpirvOps.h"

#include <stdlib.h>

#define OPCODE_MAX 256 // highest opcode supported

typedef struct vm_context
{
    op_handler* opHandlers[OPCODE_MAX];
} VM_Context;

inline void ResetContext(VM_Context* context)
{
    for (uint32 i = 0; i < OPCODE_MAX; ++i)
    {
        context->opHandlers[i] = OpHandler_Stub;
    }
}

// Enable only debug/setup operations
inline void ResetContextForSetup(VM_Context* context)
{
    ResetContext(context);

    // Enable debug functions only
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
    //context->opHandlers[SpvOpVariable] = OpHandler_OpVariable;
    context->opHandlers[SpvOpLabel] = OpHandler_OpLabel;
}

// Enable only operations for shader execution
inline void ResetContextForExecution(VM_Context* context)
{
    ResetContext(context);

    //context->opHandlers[SpvOpExtInst] = OpHandler_OpExtInst;
    //context->opHandlers[SpvOpLoad] = OpHandler_OpLoad;
    //context->opHandlers[SpvOpStore] = OpHandler_OpStore;
    //context->opHandlers[SpvOpCompositeConstruct] = OpHandler_OpCompositeConstruct;
    //context->opHandlers[SpvOpCompositeExtract] = OpHandler_OpCompositeExtract;
    //context->opHandlers[SpvOpVectorTimesScalar] = OpHandler_OpVectorTimesScalar;
    //context->opHandlers[SpvOpDot] = OpHandler_OpDot;
    //context->opHandlers[SpvOpReturn] = OpHandler_OpReturn;
}

inline VM_Context* CreateContext_Internal()
{
    VM_Context* context = (VM_Context*)malloc(sizeof(VM_Context));
    ResetContext(context);
    return context;

    // Add supported opcode handlers

    /*
    Original list
    context->opHandlers[SpvOpSource] = OpHandler_OpSource;
    context->opHandlers[SpvOpName] = OpHandler_OpName;
    context->opHandlers[SpvOpExtInstImport] = OpHandler_OpExtInstImport;
    //context->opHandlers[SpvOpExtInst] = OpHandler_OpExtInst;
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
    //context->opHandlers[SpvOpFunction] = OpHandler_OpFunction;
    //context->opHandlers[SpvOpFunctionEnd] = OpHandler_OpFunctionEnd;
    //context->opHandlers[SpvOpVariable] = OpHandler_OpVariable;
    //context->opHandlers[SpvOpLoad] = OpHandler_OpLoad;
    //context->opHandlers[SpvOpStore] = OpHandler_OpStore;
    context->opHandlers[SpvOpDecorate] = OpHandler_OpDecorate;
    //context->opHandlers[SpvOpCompositeConstruct] = OpHandler_OpCompositeConstruct;
    //context->opHandlers[SpvOpCompositeExtract] = OpHandler_OpCompositeExtract;
    context->opHandlers[SpvOpVectorTimesScalar] = OpHandler_OpVectorTimesScalar;
    context->opHandlers[SpvOpDot] = OpHandler_OpDot;
    //context->opHandlers[SpvOpLabel] = OpHandler_OpLabel;
    //context->opHandlers[SpvOpReturn] = OpHandler_OpReturn;
    */
}

inline void DestroyContext_Internal(VM_Context* context)
{
    free(context);
}

#endif
