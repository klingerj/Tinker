#include "SpirvOps.h"
#include "VMState.h"
#include "VMShader.h"

OP_HANDLER(OpHandler_OpSource)
{
    PRINT_DEBUG("\nOpSource\n");

    uint32 sourceLang = ReadSpirvWord(insnStreamPtr);
    uint32 version = ReadSpirvWord(insnStreamPtr);

    PRINT_DEBUG("source lang: %d\n", sourceLang);
    PRINT_DEBUG("version: %d\n", version);

    // TODO: do something with the optional file source operands
    uint16 numWordsRemaining = (uint16)numWordsAfterOpcode - 2;
    for (uint16 uiWord = 0; uiWord < numWordsRemaining; ++uiWord)
    {
        uint32 word = ReadSpirvWord(insnStreamPtr);
    }
}

OP_HANDLER(OpHandler_OpName)
{
    PRINT_DEBUG("\nOpName\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);

    uint16 numWordsRemaining = (uint16)numWordsAfterOpcode - 1;

    Result* result = &(state->results[id]);

    // If name is longer than previous name (probably just the 4-byte entry point literal)
    if (result->name && (numWordsRemaining * 4 > result->nameLen))
    {
        free(result->name);
        result->name = NULL;
    }

    // Allocate name is there isn't one already
    if (!result->name)
    {
        result->name = (char*)malloc(numWordsRemaining * 4);
        memset(result->name, 0, numWordsRemaining * 4);
    }

    for (uint16 uiWord = 0; uiWord < numWordsRemaining; ++uiWord)
    {
        uint32 nameChars = ReadSpirvWord(insnStreamPtr);
        for (uint8 uiChar = 0; uiChar < 4; ++uiChar)
        {
            uint8 literal = ((uint8*)&nameChars)[uiChar];
            if (literal)
            {
                result->name[uiWord * 4 + uiChar] = literal;
                PRINT_DEBUG("%c", literal);
            }
        }
    }
    PRINT_DEBUG("\n");
}

OP_HANDLER(OpHandler_OpExtInstImport)
{
    PRINT_DEBUG("\nOpExtInstImport\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);

    uint16 numWordsRemaining = (uint16)numWordsAfterOpcode - 1;
    for (uint16 uiWord = 0; uiWord < numWordsRemaining; ++uiWord)
    {
        uint32 nameChars = ReadSpirvWord(insnStreamPtr);
        for (uint8 uiChar = 0; uiChar < 4; ++uiChar)
        {
            uint8 literal = ((uint8*)&nameChars)[uiChar];
            if (literal != 0)
            {
                // TODO: store ext name
                PRINT_DEBUG("%c", literal);
            }
        }
    }
    PRINT_DEBUG("\n");
}

OP_HANDLER(OpHandler_OpMemoryModel)
{
    PRINT_DEBUG("\nOpMemoryModel\n");

    uint32 addressingModel = ReadSpirvWord(insnStreamPtr);
    uint32 memoryModel = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("addressing model: %d\n", addressingModel);
    PRINT_DEBUG("memory model: %d\n", memoryModel);
    state->ownerShader->addressingModel = (uint16)addressingModel;
    state->ownerShader->memoryModel = (uint16)memoryModel;
}

OP_HANDLER(OpHandler_OpEntryPoint)
{
    PRINT_DEBUG("\nOpEntryPoint\n");

    uint32 executionModel = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    uint32 name = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);
    PRINT_DEBUG("execution model: %d\n", executionModel);
    PRINT_DEBUG("name: %d\n", name);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_EntryPoint;
    result->data.executionModel = (uint16)executionModel;

    if (!result->name)
    {
        uint16 len = sizeof(uint32) + 1;
        result->name = (char*)malloc(len);
        result->nameLen = len;
        memset(result->name, 0, len);
    }
    memcpy(result->name, &name, sizeof(uint32));

    uint16 numWordsRemaining = (uint16)numWordsAfterOpcode - 3;
    for (uint16 uiWord = 0; uiWord < numWordsRemaining; ++uiWord)
    {
        uint32 interfaceID = ReadSpirvWord(insnStreamPtr);
        // TODO: do something with this
        PRINT_DEBUG("interface id: %d\n", interfaceID);
    }
}

OP_HANDLER(OpHandler_OpExecutionMode)
{
    PRINT_DEBUG("\nOpExecutionMode\n");

    uint32 entryPoint = ReadSpirvWord(insnStreamPtr);
    uint32 executionMode = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("entry point: %d\n", entryPoint);
    PRINT_DEBUG("execution mode: %d\n", executionMode);

    Result* result = &(state->results[entryPoint]);
    result->data.executionMode = (uint16)executionMode;

    uint16 numWordsRemaining = (uint16)numWordsAfterOpcode - 2;
    for (uint16 uiWord = 0; uiWord < numWordsRemaining; ++uiWord)
    {
        uint32 literal = ReadSpirvWord(insnStreamPtr);
        // TODO: do something with this depending on the execution mode
        PRINT_DEBUG("literal: %d\n", literal);
    }
}

OP_HANDLER(OpHandler_OpCapability)
{
    PRINT_DEBUG("\nOpCapability\n");

    uint32 capability = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("capability: %d\n", capability);
    state->ownerShader->capabilities[state->ownerShader->numCapabilities++] = (uint16)capability;
}

OP_HANDLER(OpHandler_OpTypeVoid)
{
    PRINT_DEBUG("\nOpTypeVoid\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Type;
    result->type.typeData.resultType = eResultDataType_Void;
    result->type.typeData.numElements = 0;
    result->type.typeData.memberData.elementSizeInBytes = 0;
}

OP_HANDLER(OpHandler_OpTypeFloat)
{
    PRINT_DEBUG("\nOpTypeFloat\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    uint32 bitWidth = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);
    PRINT_DEBUG("float bit width: %d\n", bitWidth);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Type;
    result->type.typeData.resultType = eResultDataType_Float;
    result->type.typeData.numElements = 1;
    result->type.typeData.memberData.elementSizeInBytes = ((uint16)bitWidth) >> 3;
}

OP_HANDLER(OpHandler_OpTypeVector)
{
    PRINT_DEBUG("\nOpTypeVector\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    uint32 componentType = ReadSpirvWord(insnStreamPtr);
    uint32 componentCount = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);
    PRINT_DEBUG("component type id: %d\n", componentType);
    PRINT_DEBUG("component count: %d\n", componentCount);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Type;
    result->type.typeData.resultType = eResultDataType_Vector;
    result->type.typeData.numElements = (uint16)componentCount;

    // Store member sizes if this vector stores a scalar type
    if (IsScalarType((ResultDataType)componentType))
    {
        result->type.typeData.memberData.memberSizesInBytes = (uint16*)malloc(sizeof(uint16) * componentCount);
        for (uint32 uiComp = 0; uiComp < componentCount; ++uiComp)
        {
            result->type.typeData.memberData.memberSizesInBytes[uiComp] =
                state->results[componentType].type.typeData.memberData.elementSizeInBytes;
        }
    }
    else
    {
        result->type.typeData.memberData.memberSizesInBytes = 0;
    }
}

OP_HANDLER(OpHandler_OpTypePointer)
{
    PRINT_DEBUG("\nOpTypePointer\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    uint32 storageClass = ReadSpirvWord(insnStreamPtr);
    uint32 typeID = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);
    PRINT_DEBUG("storage class: %d\n", storageClass);
    PRINT_DEBUG("type id: %d\n", typeID);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Type;
    result->type.typeData.resultType = eResultDataType_Pointer;
    result->type.typeData.numElements = 1;
}

OP_HANDLER(OpHandler_OpTypeFunction)
{
    PRINT_DEBUG("\nOpTypeFunction\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);

    uint32 returnTypeID = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("return type id: %d\n", returnTypeID);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Function;
    result->type.funcTypeData.returnType = (ResultDataType)returnTypeID;

    uint8 numParameters = (uint8)numWordsAfterOpcode - 2;
    result->type.funcTypeData.numParameters = numParameters;

    // Parameter type IDs
    for (uint32 uiWord = 0; uiWord < numParameters; ++uiWord)
    {
        result->type.funcTypeData.parameters[uiWord] = (ResultDataType)ReadSpirvWord(insnStreamPtr);
    }
}

OP_HANDLER(OpHandler_OpConstant)
{
    PRINT_DEBUG("\nOpConstant\n");

    uint32 resultType = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d\n", resultType);

    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);
    
    uint32 value = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("value: %d\n", value);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Constant;
    result->type.typeData.numElements = 1;
    result->type.typeData.resultType = state->results[resultType].type.typeData.resultType;
    if (IsScalarType(result->type.typeData.resultType))
    {
        result->type.typeData.memberData.elementSizeInBytes =
            state->results[resultType].type.typeData.memberData.elementSizeInBytes;
    }

    result->data.memberList.members = (Member*)malloc(sizeof(Member) * result->type.typeData.numElements);
    memcpy(result->data.memberList.members, &value, result->type.typeData.memberData.elementSizeInBytes * result->type.typeData.numElements);
    /*for (uint32 uiWord = 0; uiWord < result->type.typeData.numElements; ++uiWord)
    {
        result->data.memberList.members[uiWord];
    */
}

OP_HANDLER(OpHandler_OpConstantComposite)
{

}

OP_HANDLER(OpHandler_OpFunction)
{

}

OP_HANDLER(OpHandler_OpFunctionEnd)
{

}

OP_HANDLER(OpHandler_OpVariable)
{

}

OP_HANDLER(OpHandler_OpLoad)
{

}

OP_HANDLER(OpHandler_OpDecorate)
{

}

OP_HANDLER(OpHandler_OpLabel)
{

}

OP_HANDLER(OpHandler_OpReturn)
{

}
