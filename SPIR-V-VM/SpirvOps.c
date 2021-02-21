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

    uint16 numChars = CountChars(*insnStreamPtr);

    Result* result = &(state->results[id]);
    result->name = (char*)malloc(numChars);
    result->nameLen = numChars;
    memset(result->name, 0, numChars);
    memcpy(result->name, *insnStreamPtr, result->nameLen);
    PRINT_DEBUG("name: %s\n", result->name);

    // Then skip the actual words in the insn for the name
    uint16 numNameWordsRead = ((result->nameLen + sizeof(uint32) - 1) / sizeof(uint32));
    ConsumeSpirvWords(insnStreamPtr, numNameWordsRead);
}

OP_HANDLER(OpHandler_OpExtInst)
{
    uint32 resultTypeID = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    uint32 extSetID = ReadSpirvWord(insnStreamPtr);
    uint32 insnID = ReadSpirvWord(insnStreamPtr);

    Result* extSetResult = &(state->results[extSetID]);
    GetExtOpHandler(context, extSetResult->name, (uint16)insnID)(context, state, insnStreamPtr, numWordsAfterOpcode - 4, id, resultTypeID);
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

    switch (executionModel)
    {
        case 4:
        {
            // Fragment
            // TODO: how to handle multiple fragment outputs? MRT?
            state->outputData[0] = malloc(4 * sizeof(float));
            break;
        }

        default:
        {
            // TODO: support other execution models
            break;
        }
    }

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
    result->type.typeData.typeSizeInBytes = bitWidth / 8;
    // No sub-type data
}

OP_HANDLER(OpHandler_OpTypeVector)
{
    PRINT_DEBUG("\nOpTypeVector\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    uint32 componentTypeID = ReadSpirvWord(insnStreamPtr);
    uint32 componentCount = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);
    PRINT_DEBUG("component type id: %d, count: %d\n", componentTypeID, componentCount);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Type;
    result->type.typeData.resultType = eResultDataType_Vector;
    result->type.typeData.numElements = (uint16)componentCount;

    // Vectors only store scalar types, and all elements are the same type.
    // To save memory, only allocate one type ID.
    result->type.typeData.subTypes = (ResultID*)malloc(sizeof(ResultID));
    result->type.typeData.subTypes[0] = (ResultID)componentTypeID;
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
    result->type.typeData.subTypes = (ResultID*)malloc(sizeof(ResultID));
    result->type.typeData.subTypes[0] = (ResultID)typeID;
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
    if (numParameters > MAX_FUNC_PARAMETERS)
    {
        PRINT_ERR("More than %d function parameters declared!\n", MAX_FUNC_PARAMETERS);
        return;
    }

    // Parameter type IDs
    for (uint32 uiWord = 0; uiWord < numParameters; ++uiWord)
    {
        result->type.funcTypeData.parameters[uiWord] = (ResultDataType)ReadSpirvWord(insnStreamPtr);
    }
}

OP_HANDLER(OpHandler_OpConstant)
{
    PRINT_DEBUG("\nOpConstant\n");

    uint32 resultTypeID = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    uint32 value = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d, id: %d\n", resultTypeID, id);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Constant;
    //result->type.typeData.numElements = 1;
    //result->type.typeData.resultType = state->results[resultType].type.typeData.resultType;

    if (result->type.typeData.resultType == eResultDataType_Float)
    {
        PRINT_DEBUG("float value: %.9g\n", *(float*)&value);
    }
    else
    {
        PRINT_DEBUG("integer value: %d\n", value);
    }

    uint32 memberListSizeInBytes = sizeof(Member) * 1;
    result->data.constantVariableData.memberList.members = (Member*)malloc(memberListSizeInBytes);
    result->data.constantVariableData.memberList.members->f32 = *(float*)&value; // initialize data

    result->data.constantVariableData.typeResultID = (ResultID)resultTypeID; // reference to type id
}

OP_HANDLER(OpHandler_OpConstantComposite)
{
    PRINT_DEBUG("\nOpConstantComposite\n");

    uint32 resultTypeID = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d, id: %d\n", resultTypeID, id);

    uint16 numConstituents = numWordsAfterOpcode - 2;

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Constant;
    result->data.constantVariableData.typeResultID = (ResultID)resultTypeID;
    result->data.constantVariableData.memberList.members = (Member*)malloc(sizeof(Member) * numConstituents);

    for (uint32 uiWord = 0; uiWord < numConstituents; ++uiWord)
    {
        uint32 constituentID = ReadSpirvWord(insnStreamPtr);
        PRINT_DEBUG("constituent id: %d\n", constituentID);
        
        Result* constituentResult = &(state->results[constituentID]);
        Result* constituentTypeResult = &(state->results[constituentResult->data.constantVariableData.typeResultID]);

        if (constituentTypeResult->type.typeData.resultType == eResultDataType_Float)
        {
            // Copy the member data over
            memcpy(
            result->data.constantVariableData.memberList.members + uiWord,
            constituentResult->data.constantVariableData.memberList.members,
            sizeof(Member));
            // TODO: make this less ugly
        }
        else
        {
            PRINT_ERR("Unsupported composite constant type at this time.\n");
            return;
        }
    }
}

OP_HANDLER(OpHandler_OpFunction)
{
    PRINT_DEBUG("\nOpFunction\n");

    uint32 resultTypeID = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d, id: %d\n", resultTypeID, id);

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
    result->data.functionEntryPointData.funcReturnTypeID = (ResultID)resultTypeID;
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
    // Don't execute OpVariable's while processing function instructions during setup
    if (state->functionData.currFuncSetup)
    {
        OpHandler_Stub(context, state, insnStreamPtr, numWordsAfterOpcode);
        return;
    }

    PRINT_DEBUG("\nOpVariable\n");

    uint32 resultTypeID = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    uint32 storageClass = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d, id: %d, storage class: %d\n", resultTypeID, id, storageClass);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Variable;
    result->data.constantVariableData.typeResultID = (ResultID)resultTypeID;
    result->data.constantVariableData.storageClass = (uint16)storageClass;

    // Chase the type pointer and allocate the memory
    Result* typeResult = &(state->results[resultTypeID]);
    Result* pointedTypeResult = &(state->results[typeResult->type.typeData.subTypes[0]]);
    uint32 numElements = pointedTypeResult->type.typeData.numElements;

    // TODO: test / make this more robust maybe, e.g. make it work for structs. I think we need to make a recursive function to walk the type hierarchy
    result->data.constantVariableData.memberList.members = (Member*)malloc(sizeof(Member) * numElements);

    // Initializer if one is provided
    int16 numWordsRemaining = numWordsAfterOpcode - 3;
    if (numWordsRemaining > 0)
    {
        // TODO: untested path
        uint32 initializerID = ReadSpirvWord(insnStreamPtr);
        PRINT_DEBUG("optional initializer id: %d\n", initializerID);

        Result* initResult = &(state->results[initializerID]);
        memcpy(
        result->data.constantVariableData.memberList.members,
        initResult->data.constantVariableData.memberList.members,
        sizeof(Member) * numElements);
    }
}

OP_HANDLER(OpHandler_OpLoad)
{
    PRINT_DEBUG("\nOpLoad\n");

    uint32 resultTypeID = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    uint32 ptrID = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d, id: %d, pointer id: %d\n", resultTypeID, id, ptrID);

    Result* varResult = &(state->results[ptrID]);
    Result* varTypeResult = &(state->results[varResult->data.constantVariableData.typeResultID]);
    Result* varPointedTypeResult = &(state->results[varTypeResult->type.typeData.subTypes[0]]);
    // TODO: test / make this more robust maybe, e.g. make it work for structs. I think we need to make a recursive function to walk the type hierarchy?

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Constant;

    uint32 numVarDataElements = varPointedTypeResult->type.typeData.numElements;
    result->data.constantVariableData.memberList.members = (Member*)malloc(sizeof(Member) * numVarDataElements);

    // Check storage class to determine where to load from
    switch (varResult->data.constantVariableData.storageClass)
    {
        case 1:
        {
            // Find the storage decoration
            uint8 decIndex = MAX_DECORATIONS;
            for (uint8 i = 0; i < varResult->numDecorations; ++i)
            {
                if (varResult->decorations[i].type == (uint16)SpvDecorationLocation)
                {
                    decIndex = i;
                    break;
                }
            }
            Decoration* storageDec = &varResult->decorations[decIndex];

            // Pipeline input - populate member data with input data
            for (uint16 uiMember = 0; uiMember < numVarDataElements; ++uiMember)
            {
                // TODO: have to check the type here to know which member to use
                result->data.constantVariableData.memberList.members[uiMember].f32 = ((float*)state->inputData[storageDec->literals[0]])[uiMember];
            }
            break;
        }

        case 3:
        {
            // Pipeline output - set output pointer equal to variable pointer
            // TODO: set the output pointer?
            // TODO: this probably shouldn't happen on output variables?
            // I think you can technically read from output vars maybe? Or is that undefined?

            // TODO: find the decoration and get the literal like above for the storage location index
            //state->outputData[0] = varResult->data.constantVariableData.memberList.members;
            break;
        }

        //case 7:
        //{
            // Function scope
            // TODO: could use a stack memory allocator here for variables like this to free their data easily
        //    break;
        //}

        default:
        {
            for (uint16 uiMember = 0; uiMember < numVarDataElements; ++uiMember)
            {
                result->data.constantVariableData.memberList.members[uiMember].f32 = varResult->data.constantVariableData.memberList.members[uiMember].f32;
            }

            //PRINT_ERR("Unsupported storage class: %d\n", storageClass);
            break;
        }
    }

    int16 numWordsRemaining = numWordsAfterOpcode - 3;
    if (numWordsRemaining > 0)
    {
        uint32 memMask = ReadSpirvWord(insnStreamPtr);
        PRINT_DEBUG("optional memory operand mask: %d\n", memMask);
    }
}

OP_HANDLER(OpHandler_OpStore)
{
    PRINT_DEBUG("\nOpStore\n");

    uint32 ptrID = ReadSpirvWord(insnStreamPtr);
    uint32 objectID = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("pointer id: %d, object id: %d\n", ptrID, objectID);

    Result* ptrResult = &(state->results[ptrID]);
    Result* objectResult = &(state->results[objectID]);
    Result* objectTypeResult = &(state->results[objectResult->data.constantVariableData.typeResultID]);

    switch (ptrResult->data.constantVariableData.storageClass)
    {
        case 1:
        {
            // This definitely shouldn't happen :)
            break;
        }

        case 3:
        {
            // Find the storage decoration
            uint8 decIndex = MAX_DECORATIONS;
            for (uint8 i = 0; i < ptrResult->numDecorations; ++i)
            {
                if (ptrResult->decorations[i].type == (uint16)SpvDecorationLocation)
                {
                    decIndex = i;
                    break;
                }
            }
            Decoration* storageDec = &ptrResult->decorations[decIndex];

            // Pipeline output - populate member data with input data
            for (uint16 uiMember = 0; uiMember < objectTypeResult->type.typeData.numElements; ++uiMember)
            {
                // TODO: have to check the type here to know which member to use
                ((float*)state->outputData[storageDec->literals[0]])[uiMember] = objectResult->data.constantVariableData.memberList.members[uiMember].f32;
            }

            break;
        }

        default:
        {
            for (uint16 uiMember = 0; uiMember < objectTypeResult->type.typeData.numElements; ++uiMember)
            {
                // TODO: have to check the type here to know which member to use
                ptrResult->data.constantVariableData.memberList.members[uiMember].f32 = objectResult->data.constantVariableData.memberList.members[uiMember].f32;
            }

            break;
        }
    }

    int16 numWordsRemaining = numWordsAfterOpcode - 2;
    if (numWordsRemaining > 0)
    {
        // TODO: untested path
        uint32 memMask = ReadSpirvWord(insnStreamPtr);
        PRINT_DEBUG("optional memory operand mask: %d\n", memMask);
    }
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
    PRINT_DEBUG("\nOpCompositeConstruct\n");

    uint32 resultTypeID = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d, id: %d\n", resultTypeID, id);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Constant;
    result->data.constantVariableData.typeResultID = (ResultID)resultTypeID;

    //Result* dataTypeResult = &(state->results[result->data.constantVariableData.typeResultID]);
    Result* dataTypeResult = &(state->results[resultTypeID]);

    // TODO: test / make this more robust maybe, e.g. make it work for structs. I think we need to make a recursive function to walk the type hierarchy
    result->data.constantVariableData.memberList.members = (Member*)malloc(sizeof(Member) * dataTypeResult->type.typeData.numElements);

    int16 numWordsRemaining = numWordsAfterOpcode - 2;
    for (int16 iWord = 0; iWord < numWordsRemaining; ++iWord)
    {
        uint32 constituentID = ReadSpirvWord(insnStreamPtr);
        PRINT_DEBUG("consituent id: %d\n", constituentID);

        Result* constResult = &(state->results[constituentID]);
        result->data.constantVariableData.memberList.members[iWord] = *(constResult->data.constantVariableData.memberList.members);
    }
}

OP_HANDLER(OpHandler_OpCompositeExtract)
{
    PRINT_DEBUG("\nOpCompositeExtract\n");

    uint32 resultTypeID = ReadSpirvWord(insnStreamPtr);
    uint32 id = ReadSpirvWord(insnStreamPtr);
    uint32 compositeID = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("result type id: %d, id: %d, composite id: %d\n", resultTypeID, id, compositeID);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_Constant;
    result->data.constantVariableData.typeResultID = (ResultID)resultTypeID;
    
    Result* typeResult = &(state->results[resultTypeID]);

    uint32 numMembersAfterIndexing = 0;
    if (typeResult->type.typeData.resultType == eResultDataType_Float)
    {
        numMembersAfterIndexing = 1;
    }
    else
    {
        PRINT_ERR("Unsupported composite type to index.\n");
        return;
    }
    result->data.constantVariableData.memberList.members = (Member*)malloc(sizeof(Member) * numMembersAfterIndexing);

    Result* compResult = &(state->results[compositeID]);

    int16 numWordsRemaining = numWordsAfterOpcode - 3;
    for (int16 iWord = 0; iWord < numWordsRemaining; ++iWord)
    {
        uint32 index = ReadSpirvWord(insnStreamPtr);
        PRINT_DEBUG("index: %d\n", index);

        // TODO: this should be smarter logic, e.g. need to walk the type hierarchy
        result->data.constantVariableData.memberList.members[iWord] = compResult->data.constantVariableData.memberList.members[index];
    }
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
    result->resultType = eResultType_Constant; // TODO: check if this should be a variable or constant

    //Result* typeResult = &(state->results[resultTypeID]);

    Result* scalar = &(state->results[scalarID]);
    Result* vector = &(state->results[vectorID]);
    Result* vectorTypeResult = &(state->results[vector->data.constantVariableData.typeResultID]);
    uint16 numElements = vectorTypeResult->type.typeData.numElements;

    MemberList* memberList = &result->data.constantVariableData.memberList;
    if (!memberList->members)
    {
        PRINT_DEBUG("No member data already allocated for dst ID in OpVectorTimeScalar.\n");
        memberList->members = (Member*)malloc(sizeof(Member) * numElements);
    }
    result->data.constantVariableData.typeResultID = (ResultID)resultTypeID;

    for (uint16 uiEle = 0; uiEle < numElements; ++uiEle)
    {
        // This op assumes floats
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
    result->resultType = eResultType_Constant; // TODO: check if this should be a variable or constant

    uint32 vector1ID = ReadSpirvWord(insnStreamPtr);
    uint32 vector2ID = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("vector 1 id: %d, vector 2 id: %d\n", vector1ID, vector2ID);

    Result* vector1 = &(state->results[vector1ID]);
    Result* vector2 = &(state->results[vector2ID]);
    Result* vectorTypeResult = &(state->results[vector1->data.constantVariableData.typeResultID]);

    float dotProdSum = 0.0f;
    for (uint16 uiEle = 0; uiEle < vectorTypeResult->type.typeData.numElements; ++uiEle)
    {
        float v1 = vector1->data.constantVariableData.memberList.members[uiEle].f32;
        float v2 = vector2->data.constantVariableData.memberList.members[uiEle].f32;
        dotProdSum += v1 * v2;
    }

    MemberList* memberList = &result->data.constantVariableData.memberList;
    if (!memberList->members)
    {
        memberList->members = (Member*)malloc(sizeof(Member) * 1); // dot product results in one final float
    }
    memberList->members[0].f32 = dotProdSum;
    result->data.constantVariableData.typeResultID = (ResultID)resultTypeID;
}

OP_HANDLER(OpHandler_OpLabel)
{
    PRINT_DEBUG("\nOpLabel\n");

    uint32 id = ReadSpirvWord(insnStreamPtr);
    PRINT_DEBUG("id: %d\n", id);

    Result* result = &(state->results[id]);
    result->resultType = eResultType_BlockLabel;
    result->data.BlockLabelData.insnBlockPtr = *insnStreamPtr;
}

OP_HANDLER(OpHandler_OpReturn)
{
    PRINT_DEBUG("\nOpReturn\n");
    // TODO: go back in the function call stack
}
