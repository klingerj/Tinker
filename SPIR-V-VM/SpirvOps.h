#ifndef SPIV_OPS_H
#define SPIV_OPS_H

#include "CoreDefines.h"

struct VM_State;
#define OP_HANDLER(name) void name(VM_State* state, uint16 numWordsAfterOpcode)
typedef OP_HANDLER(op_handler);
inline OP_HANDLER(OpHandler_Stub)
{
    // Unsupported operation
    PRINT_ERR("Unsupported opcode.\n");
}

OP_HANDLER(OpHandler_OpSource); // 3
OP_HANDLER(OpHandler_OpName); // 5
OP_HANDLER(OpHandler_OpExtInstImport); // 11
OP_HANDLER(OpHandler_OpMemoryModel); // 14
OP_HANDLER(OpHandler_OpEntryPoint); // 15
OP_HANDLER(OpHandler_OpExecutionMode); // 16
OP_HANDLER(OpHandler_OpCapability); // 17
OP_HANDLER(OpHandler_OpTypeVoid); // 19
OP_HANDLER(OpHandler_OpTypeFloat); // 22
OP_HANDLER(OpHandler_OpTypeVector); // 23
OP_HANDLER(OpHandler_OpTypePointer); // 32
OP_HANDLER(OpHandler_OpTypeFunction); // 33
OP_HANDLER(OpHandler_OpConstant); // 43
OP_HANDLER(OpHandler_OpConstantComposite); // 44
OP_HANDLER(OpHandler_OpFunction); // 54
OP_HANDLER(OpHandler_OpFunctionEnd); // 56
OP_HANDLER(OpHandler_OpVariable); // 59
OP_HANDLER(OpHandler_OpLoad); // 61
OP_HANDLER(OpHandler_OpDecorate); // 71
OP_HANDLER(OpHandler_OpLabel); // 248
OP_HANDLER(OpHandler_OpReturn); // 253

#endif
