#include "SpirvOps.h"
#include "VMState.h"
#include "VMShader.h"

OP_HANDLER(OpHandler_OpSource)
{
    PRINT_DEBUG("\nOpSource\n");

    uint32 sourceLang = ReadSpirvWord(insnStreamPtr);
    uint32 version = ReadSpirvWord(insnStreamPtr);

    PRINT_DEBUG("source lang: %d, version: %d\n", sourceLang, version);

    // TODO: do something with the optional file source operands
    uint16 numWordsRemaining = numWordsAfterOpcode - 2;
    for (uint16 uiWord = 0; uiWord < numWordsRemaining; ++uiWord)
    {
        //uint32 word = ReadSpirvWord(insnStreamPtr);
        CONSUME_SPIRV_WORD(insnStreamPtr);
    }
}

OP_HANDLER(OpHandler_OpName)
{
    PRINT_DEBUG("\nOpName\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d, ", id);

    uint16 numWordsRemaining = numWordsAfterOpcode - 1;

    Result* result = &(state->results[id]);

    // If name is different from previous name in length or contents
    uint16 numChars = CountChars(*insnStreamPtr);

    // If the result already has a name, check if it's identical to the new one provided in this command
    if (result->name)
    {
        uint8 isNameIdentical = (numChars == result->nameLen) || strncmp(result->name, (char*)*insnStreamPtr, numChars) == 0;
        if (isNameIdentical)
        {
            // Name is identical - do nothing
            ConsumeSpirvWords(insnStreamPtr, numWordsRemaining);
            PRINT_DEBUG("name: %s\n", result->name);
            return;
        }
        else
        {
            // Going to replace with new name
            free(result->name);
            result->name = NULL;
            result->nameLen = 0;
        }
    }

    // Allocate name is there isn't one already
    if (!result->name)
    {
        result->name = (char*)malloc(numChars);
        result->nameLen = numChars;
        memset(result->name, 0, numChars);
        memcpy(result->name, *insnStreamPtr, result->nameLen);
        PRINT_DEBUG("name: %s\n", result->name);

        // Then skip the actual words in the insn for the name
        uint16 numNameWordsRead = ((result->nameLen + sizeof(uint32) - 1) / sizeof(uint32));
        ConsumeSpirvWords(insnStreamPtr, numNameWordsRead);
    }
}

OP_HANDLER(OpHandler_OpExtInstImport)
{
    PRINT_DEBUG("\nOpExtInstImport\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d, ext name: ", id);

    uint16 numWordsRemaining = numWordsAfterOpcode - 1;
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

OP_HANDLER(OpHandler_OpExtInst)
{

}

OP_HANDLER(OpHandler_OpMemoryModel)
{
    PRINT_DEBUG("\nOpMemoryModel\n");

    uint32 addressingModel = ReadSpirvWord(insnStreamPtr);
    uint32 memoryModel = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("addressing model: %d, memory model: %d\n", addressingModel, memoryModel);
    state->ownerShader->addressingModel = (uint16)addressingModel;
    state->ownerShader->memoryModel = (uint16)memoryModel;
}

OP_HANDLER(OpHandler_OpEntryPoint)
{
    PRINT_DEBUG("\nOpEntryPoint\n");

    uint32 executionModel = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d, execution model: %d\n", id, executionModel);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_EntryPoint;
    result->data.functionEntryPointData.executionModel = (uint16)executionModel;

    // Read chars until we see a null terminator - count number
    if (!result->name)
    {
        uint16 nameLen = CountChars(*insnStreamPtr);
        result->name = (char*)malloc(nameLen);
        result->nameLen = nameLen;
        memset(result->name, 0, nameLen);
        memcpy(result->name, *insnStreamPtr, nameLen);
        PRINT_DEBUG("name: %s\n", result->name);
    }
    // Then skip the actual words in the insn for the name
    uint16 numNameWordsRead = ((result->nameLen + sizeof(uint32) - 1) / sizeof(uint32));
    ConsumeSpirvWords(insnStreamPtr, numNameWordsRead);

    uint16 numWordsRemaining = numWordsAfterOpcode - (2 + numNameWordsRead);
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
    PRINT_DEBUG("entry point: %d, execution mode: %d\n", entryPoint, executionMode);

    Result* result = &(state->results[entryPoint]);
    result->data.functionEntryPointData.executionMode = (uint16)executionMode;

    uint16 numWordsRemaining = numWordsAfterOpcode - 2;
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
    PRINT_DEBUG("id: %d, float bit width: %d\n", id, bitWidth);

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
    PRINT_DEBUG("component type id: %d, count: %d\n", componentType, componentCount);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Type;
    result->type.typeData.resultType = eResultDataType_Vector;
    result->type.typeData.numElements = (uint16)componentCount;

    // Store member sizes if this vector stores a scalar type
    if (IsScalarType((ResultDataType)componentType))
    {
        result->type.typeData.memberData.memberSizesInBytes = (uint16*)malloc(sizeof(uint16) * componentCount);
        result->type.typeData.memberData.memberTypes = (ResultID*)malloc(sizeof(ResultID) * componentCount);
        for (uint32 uiComp = 0; uiComp < componentCount; ++uiComp)
        {
            result->type.typeData.memberData.memberSizesInBytes[uiComp] =
                state->results[componentType].type.typeData.memberData.elementSizeInBytes;
            result->type.typeData.memberData.memberTypes[uiComp] = (ResultID)componentType;
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
    PRINT_DEBUG("id: %d, storage class: %d, type id: %d\n", id, storageClass, typeID);

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
    uint32 id = ReadSpirvWord(insnStreamPtr);
    uint32 value = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d, id: %d\n", resultType, id);
    if (state->results[resultType].type.typeData.resultType == eResultDataType_Float)
    {
        PRINT_DEBUG("float value: %.9g\n", *(float*)&value);
    }
    else
    {
        PRINT_DEBUG("integer value: %d\n", value);
    }

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Constant;
    result->type.typeData.numElements = 1;
    result->type.typeData.resultType = state->results[resultType].type.typeData.resultType;
    result->type.typeData.memberData.elementSizeInBytes = state->results[resultType].type.typeData.memberData.elementSizeInBytes;

    result->data.constantVariableData.memberList.members = (Member*)malloc(sizeof(Member) * result->type.typeData.numElements);
    memcpy(result->data.constantVariableData.memberList.members, &value, result->type.typeData.memberData.elementSizeInBytes * result->type.typeData.numElements);
}

OP_HANDLER(OpHandler_OpConstantComposite)
{
    PRINT_DEBUG("\nOpConstantComposite\n");

    uint32 resultType = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d, id: %d\n", resultType, id);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Constant;
    result->type.typeData.resultType = state->results[resultType].type.typeData.resultType;

    uint16 numConstituents = numWordsAfterOpcode - 2;
    result->type.typeData.numElements = numConstituents;
    result->type.typeData.memberData.memberTypes = (ResultID*)malloc(sizeof(ResultID) * numConstituents);
    result->type.typeData.memberData.memberSizesInBytes = (uint16*)malloc(sizeof(ResultID) * numConstituents);

    // Read each constituent and determine type and constant data info
    const uint32* insnStreamPtrBase = *insnStreamPtr;

    uint32 totalMemberDataSize = 0;
    for (uint32 uiWord = 0; uiWord < numConstituents; ++uiWord)
    {
        uint32 constituentID = ReadSpirvWord(&insnStreamPtrBase);
        PRINT_DEBUG("constituent id: %d\n", constituentID);
        Result* constituentResult = &(state->results[constituentID]);

        result->type.typeData.memberData.memberTypes[uiWord] = constituentResult->type.typeData.resultType;

        if (IsScalarType(constituentResult->type.typeData.resultType))
        {
            result->type.typeData.memberData.memberSizesInBytes[uiWord] = constituentResult->type.typeData.memberData.elementSizeInBytes;
            totalMemberDataSize += constituentResult->type.typeData.memberData.elementSizeInBytes;
        }
        else
        {
            // Aggregate type
            // TODO: decide what to do here
            result->type.typeData.memberData.memberSizesInBytes[uiWord] = 0;// constituentResult->type.typeData.memberData.;
        }
    }

    // Re-read the constituents for data, actually advance the insn stream pointer this time
    result->data.constantVariableData.memberList.members = (Member*)malloc(totalMemberDataSize);
    for (uint32 uiEle = 0; uiEle < numConstituents; ++uiEle)
    {
        uint32 constituentID = ReadSpirvWord(insnStreamPtr);
        Result* constituentResult = &(state->results[constituentID]);

        if (IsScalarType(constituentResult->type.typeData.resultType))
        {
            uint32 dataSizeInBytes = constituentResult->type.typeData.numElements * constituentResult->type.typeData.memberData.elementSizeInBytes;
            memcpy(result->data.constantVariableData.memberList.members, constituentResult->data.constantVariableData.memberList.members, dataSizeInBytes);
        }
        else
        {
            // TODO: decide what to do here, do we store IDs or no?
        }
    }
}

OP_HANDLER(OpHandler_OpFunction)
{
    PRINT_DEBUG("\nOpFunction\n");

    uint32 resultType = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d, id: %d\n", resultType, id);

    uint32 funcControlMask = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("func control mask: %d\n", funcControlMask);

    uint32 funcTypeID = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("func type id: %d\n", funcTypeID);

    Result* result = &(state->results[id]);

    // Set to function type if not already marked as an entry point
    if (result->resultType != eResultType_EntryPoint)
    {
        result->resultType = eResultType_Function;
    }
    result->data.functionEntryPointData.funcReturnTypeID = (ResultID)resultType;
    result->data.functionEntryPointData.funcTypeID = (ResultID)funcTypeID;
    
    // grab current insn ptr as where to jump to when this function is called
    result->data.functionEntryPointData.insnPtr = *insnStreamPtr;

    state->functionData.currFuncSetup = result;
}

OP_HANDLER(OpHandler_OpFunctionEnd)
{
    PRINT_DEBUG("\nOpFunctionEnd\n");
    state->functionData.currFuncSetup = NULL;
}

OP_HANDLER(OpHandler_OpVariable)
{

}

OP_HANDLER(OpHandler_OpLoad)
{

}

OP_HANDLER(OpHandler_OpStore)
{

}

OP_HANDLER(OpHandler_OpDecorate)
{
    PRINT_DEBUG("\nOpDecorate\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);

    Result* result = &(state->results[id]);

    for (int16 uiWordsToProcess = numWordsAfterOpcode - 1; uiWordsToProcess > 0;)
    {
        Decoration newDec;
        memset(&newDec, 0, sizeof(Decoration));

        uint32 decType = ReadSpirvWord(insnStreamPtr);
        PRINT_DEBUG("Decoration type: %d\n", decType);
        newDec.type = (uint16)decType;
        --uiWordsToProcess;

        uint16 numDecorationLiterals = NumDecorationLiterals(newDec.type);
        for (uint16 uiLiteral = 0; uiLiteral < numDecorationLiterals; ++uiLiteral)
        {
            uint32 decorationLiteral = ReadSpirvWord(insnStreamPtr);
            PRINT_DEBUG("Decoration literal: %d\n", decorationLiteral);
            newDec.literals[uiLiteral] = (uint16)decorationLiteral;

            --uiWordsToProcess;
        }

        result->decorations[result->numDecorations++] = newDec;
    }
}

OP_HANDLER(OpHandler_OpCompositeConstruct)
{

}

OP_HANDLER(OpHandler_OpCompositeExtract)
{

}

OP_HANDLER(OpHandler_OpVectorTimesScalar)
{
    PRINT_DEBUG("\nOpVectorTimesScalar\n");

    uint32 resultTypeID = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d\n", resultTypeID);

    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);

    uint32 vectorID = ReadSpirvWord(insnStreamPtr);
    uint32 scalarID = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("vector id: %d, scalar id: %d\n", vectorID, scalarID);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Variable; // TODO: make sure this is right
    result->type.typeData.resultType = eResultDataType_Float;
    result->type.typeData.memberData.elementSizeInBytes = sizeof(float);

    Result* scalar = &(state->results[scalarID]);
    Result* vector = &(state->results[vectorID]);
    uint16 numElements = vector->type.typeData.numElements;
    result->type.typeData.numElements = numElements;

    MemberList* memberList = &result->data.constantVariableData.memberList;
    if (!memberList->members)
    {
        memberList->members = (Member*)malloc(sizeof(Member) * numElements);
    }

    for (uint16 uiEle = 0; uiEle < numElements; ++uiEle)
    {
        float v = vector->data.constantVariableData.memberList.members[uiEle].f32;
        v *= scalar->data.constantVariableData.memberList.members[0].f32;
        memberList->members[uiEle].f32 = v;
    }
}

OP_HANDLER(OpHandler_OpDot)
{
    PRINT_DEBUG("\nOpDot\n");

    uint32 resultTypeID = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d\n", resultTypeID);

    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Variable; // TODO: make sure this is right
    result->type.typeData.numElements = 1;
    result->type.typeData.resultType = eResultDataType_Float;
    result->type.typeData.memberData.elementSizeInBytes = sizeof(float);

    uint32 vector1ID = ReadSpirvWord(insnStreamPtr);
    uint32 vector2ID = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("vector 1 id: %d, vector 2 id: %d\n", vector1ID, vector2ID);

    Result* vector1 = &(state->results[vector1ID]);
    Result* vector2 = &(state->results[vector2ID]);

    uint16 numElements = vector1->type.typeData.numElements;
    float dotProdSum = 0.0f;
    for (uint16 uiEle = 0; uiEle < numElements; ++uiEle)
    {
        float v1 = vector1->data.constantVariableData.memberList.members[uiEle].f32;
        float v2 = vector2->data.constantVariableData.memberList.members[uiEle].f32;
        dotProdSum += v1 * v2;
    }

    MemberList* memberList = &result->data.constantVariableData.memberList;
    if (!memberList->members)
    {
        memberList->members = (Member*)malloc(sizeof(Member) * result->type.typeData.numElements);
    }
    memberList->members->f32 = dotProdSum;
}

OP_HANDLER(OpHandler_OpLabel)
{
    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_BlockLabel;
    result->data.BlockLabelData.insnBlockPtr = *insnStreamPtr;
}

OP_HANDLER(OpHandler_OpReturn)
{

}
