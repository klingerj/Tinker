#include "VMState.h"
#include "VMShader.h"
#include "VMContext.h"

#include <string.h>

VM_State* CreateState_Internal(VM_Context* context, VM_Shader* shader)
{
    VM_State* newState = (VM_State*)malloc(sizeof(VM_State));
    memset(newState, 0, sizeof(VM_State));
    newState->ownerShader = shader;
    newState->numResultIDs = shader->boundNum;
    newState->results = (Result*)malloc(sizeof(Result) * newState->numResultIDs);
    memset(newState->results, 0, sizeof(Result) * newState->numResultIDs);

    ResetContextForSetup(context);
    const uint32* insnStream = shader->insnStream;
    const uint32* insnStreamEnd = shader->insnStream + shader->insnStreamSizeInWords;

    while (insnStream < insnStreamEnd)
    {
        uint32 opcodeAndWordCount = ReadSpirvWord(&insnStream);
        uint16 opcode = (uint16)OPCODE(opcodeAndWordCount);
        uint16 wordCount = (uint16)WORD_COUNT(opcodeAndWordCount);

        if (opcode < OPCODE_MAX)
        {
            context->opHandlers[opcode](newState, &insnStream, wordCount - 1);
        }
        else
        {
            PRINT_ERR("Opcode detected that exceeded array size.\n");
        }
    }

    return newState;
}

void DestroyState_Internal(VM_Context* context, VM_State* state)
{
    free(state->results);
    free(state);
}

ResultID FindEntryPoint(VM_State* state, const char* name)
{
    for (uint16 uiID = 0; uiID < state->numResultIDs; ++uiID)
    {
        Result* result = &(state->results[uiID]);
        if (result->resultType != eResultType_EntryPoint) { continue; }

        if ((strlen(name) == result->nameLen - 1) && (strncmp(name, result->name, result->nameLen - 1) == 0))
        {
            return uiID;
        }
    }

    return (ResultID)-1;
}

// TODO: this will probably have to change in order to handle general function calls
// e.g. dont need to reset context every time
void CallFunction(VM_Context* context, VM_State* state, ResultID functionID)
{
    ResetContextForExecution(context);

    Result* funcResult = &(state->results[functionID]);
    const uint32* insnStream = funcResult->data.functionEntryPointData.insnPtr;
    const uint32* insnStreamEnd = state->ownerShader->insnStream + state->ownerShader->insnStreamSizeInWords;

    while (insnStream < insnStreamEnd)
    {
        uint32 opcodeAndWordCount = ReadSpirvWord(&insnStream);
        uint16 opcode = (uint16)OPCODE(opcodeAndWordCount);
        uint16 wordCount = (uint16)WORD_COUNT(opcodeAndWordCount);

        if (opcode < OPCODE_MAX)
        {
            context->opHandlers[opcode](state, &insnStream, wordCount - 1);
        }
        else
        {
            PRINT_ERR("Opcode detected that exceeded array size.\n");
        }
    }
}

void AddStateInputData_Internal(VM_Context* context, VM_State* state, uint32 location, void* data, uint32 dataSizeInBytes)
{
    state->inputData[location] = malloc(dataSizeInBytes);
    memcpy(state->inputData[location], data, dataSizeInBytes);
}
