#include "VMState.h"
#include "VMShader.h"

#include <cstring>

VM_State* CreateState_Internal(VM_Context* context, VM_Shader* shader)
{
    VM_State* newState = (VM_State*)malloc(sizeof(VM_State));
    newState->ownerShader = shader;
    newState->numResultIDs = shader->boundNum;
    newState->results = (Result*)malloc(sizeof(Result) * newState->numResultIDs);
    memset(newState->results, 0, sizeof(Result) * newState->numResultIDs);

    const uint32* insnStream = shader->insnStream;

    while (insnStream < (shader->insnStream + shader->insnStreamSizeInWords))
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

    /*
    const uint32* insnPtr = shader->insnStream;

    uint32 word = *insnPtr;
    uint16 insnWordCount = (uint16)INSN_COUNT(word);
    uint16 insnOpcode = (uint16)OPCODE(word);
    ++insnPtr;

    switch (insnOpcode)
    {
        case 5:
        {
            PRINT_DEBUG("OpName instruction ignored - TODO\n");

            word = *insnPtr;
            PRINT_DEBUG("id: %d\n", word);
            ++insnPtr;

            for (uint16 uiWord = 1; uiWord < insnWordCount - 1; ++uiWord)
            {
                word = *insnPtr;
                uint8* chars = (uint8*)&word;
                PRINT_DEBUG("%c", chars[0]);
                PRINT_DEBUG("%c", chars[1]);
                PRINT_DEBUG("%c", chars[2]);
                PRINT_DEBUG("%c", chars[3]);
                ++insnPtr;
            }
            PRINT_DEBUG("\n\n");

            break;
        }

        case 19:
        {
            PRINT_DEBUG("OpTypeVoid\n");

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            // Result id is a type, that type is void
            Result_ID& result = state->resultIDs[id];
            result.type = eResultType_Type;
            result.specType = eSpecType_Void;
            PRINT_DEBUG("\n");

            break;
        }

        case 22:
        {
            PRINT_DEBUG("OpTypeFloat\n");

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            Result_ID& result = state->resultIDs[id];
            result.type = eResultType_Type;
            result.specType = eSpecType_Float;

            uint32 bitWidth = *insnPtr;
            PRINT_DEBUG("float bit width: %d\n", bitWidth);
            ++insnPtr;

            result.bitWidth = (uint8)bitWidth;

            PRINT_DEBUG("\n");

            break;
        }

        case 23:
        {
            PRINT_DEBUG("OpTypeVector\n");

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            uint32 componentType = *insnPtr;
            PRINT_DEBUG("component type id: %d\n", componentType);
            ++insnPtr;

            uint32 componentCount = *insnPtr;
            PRINT_DEBUG("component count: %d\n", componentCount);
            ++insnPtr;

            Result_ID& result = state->resultIDs[id];
            result.type = eResultType_Type;
            result.specType = eSpecType_Vector;
            result.numComponents = componentCount;
            result.componentSpecType = componentType;

            PRINT_DEBUG("\n");

            break;
        }

        case 32:
        {
            PRINT_DEBUG("OpTypePointer\n");

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            Result_ID& result = state->resultIDs[id];
            result.type = eResultType_Type;
            result.specType = eSpecType_LogicalPointer;
            // TODO: handle physical pointers?

            uint32 storageClass = *insnPtr;
            PRINT_DEBUG("storage class: %d\n", storageClass);
            ++insnPtr;

            uint32 typeID = *insnPtr;
            PRINT_DEBUG("type id: %d\n", typeID);
            ++insnPtr;

            result.storageClass = (uint16)storageClass;
            result.pointerSpecType = state->resultIDs[typeID].specType;

            PRINT_DEBUG("\n");

            break;
        }

        case 33:
        {
            PRINT_DEBUG("OpTypeFunction\n");

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            Result_ID& result = state->resultIDs[id];
            result.type = eResultType_Type;
            result.specType = eSpecType_Function;

            uint32 returnTypeID = *insnPtr;
            PRINT_DEBUG("return type: %d\n", returnTypeID);
            ++insnPtr;

            Result_ID& returnTypeResult = state->resultIDs[returnTypeID];
            if (returnTypeResult.type != eResultType_Type)
            {
                PRINT_ERR("Attempted to create function whose return type ID was not record as a type.\n");
            }
            else
            {
                result.returnType = returnTypeResult.specType;

                // Remaining parameters
                for (int16 uiWordsToProcess = insnWordCount - 3; uiWordsToProcess > 0;)
                {
                    Result_ID& paramResult = state->resultIDs[returnTypeID];
                    if (returnTypeResult.type != eResultType_Type)
                    {
                        PRINT_ERR("Attempted to create function whose return type ID was not record as a type.\n");
                    }
                    else
                    {
                        if (paramResult.specType == eSpecType_Void)
                        {
                            PRINT_ERR("Function parameter type was specified at void.\n");
                        }
                        else
                        {
                            result.parameters[result.numParameters++] = paramResult.specType;
                        }
                    }
                }
            }
            PRINT_DEBUG("\n");

            break;
        }

        case 43:
        {
            PRINT_DEBUG("OpConstant\n");

            uint32 resultType = *insnPtr;
            PRINT_DEBUG("result type id: %d\n", resultType);
            ++insnPtr;

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            Result_ID& result = state->resultIDs[id];
            result.type = eResultType_Data;
            result.specType = state->resultIDs[resultType].specType;

            uint32 value = *insnPtr;
            PRINT_DEBUG("value: %d\n", value);
            ++insnPtr;

            result.elementSizeInBytes = state->resultIDs[resultType].bitWidth / 8;
            result.numElements = 1;
            uint32 dataSize = result.elementSizeInBytes * result.numElements;
            result.data = malloc(dataSize); // TODO: allocate less terribly
            memcpy(result.data, &value, dataSize);

            // Drain remaining words
            for (uint16 uiWord = 0; uiWord < insnWordCount - 4; ++uiWord)
            {
                PRINT_ERR("Extra word in OpConstant detected - types above 32 bits not currently supported.\n");
                PRINT_ERR("Word value: %d\n", *insnPtr);
                ++insnPtr;
            }
            PRINT_DEBUG("\n");

            break;
        }

        case 44:
        {
            PRINT_DEBUG("OpConstantComposite\n");

            uint32 resultType = *insnPtr;
            PRINT_DEBUG("result type id: %d\n", resultType);
            ++insnPtr;

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            Result_ID& result = state->resultIDs[id];
            result.type = eResultType_Data;
            result.specType = state->resultIDs[resultType].specType;

            // TODO: figure out the right way to find out the bit width of the elements, either store it per element, or store a type id reference and follow it
            result.elementSizeInBytes = 4;
            result.numElements = insnWordCount - 3;
            uint32 dataSizeInBytes = result.numElements * result.elementSizeInBytes;
            result.data = malloc(dataSizeInBytes);
            for (uint16 uiWord = 0; uiWord < result.numElements; ++uiWord)
            {
                uint16 eleID = (uint16)*insnPtr;
                PRINT_DEBUG("element id: %d\n", eleID);
                Result_ID& eleResult = state->resultIDs[eleID];
                PRINT_DEBUG("element data: %d\n", *(uint32*)eleResult.data);
                // TODO: need to deduce the type here too
                memcpy((uint32*)result.data + uiWord, eleResult.data, eleResult.elementSizeInBytes * eleResult.numElements);
                ++insnPtr;
            }
            PRINT_DEBUG("\n");

            break;
        }

        case 54:
        {
            PRINT_DEBUG("OpFunction\n");

            uint32 resultType = *insnPtr;
            PRINT_DEBUG("return type id: %d\n", resultType);
            ++insnPtr;

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            uint32 funcControl = *insnPtr;
            PRINT_DEBUG("func control: %d\n", funcControl);
            ++insnPtr;

            uint32 funcTypeID = *insnPtr;
            PRINT_DEBUG("function type id: %d\n", funcTypeID);
            ++insnPtr;

            //funcTypeID
            //funcStartOffset

            Result_ID& result = state->resultIDs[id];
            result.type = eResultType_Function;
            result.specType = eSpecType_Function;
            result.funcTypeID = (uint16)funcTypeID;
            result.insnStartOffset = (uint32)(insnPtr - (state->ownerShader->insnStream)); // get offset into instruction stream

            PRINT_DEBUG("\n");

            break;
        }

        case 56:
        {
            PRINT_DEBUG("OpFunctionEnd\n");

            PRINT_DEBUG("\n");

            break;
        }

        case 59:
        {
            PRINT_DEBUG("OpVariable\n");

            uint32 resultType = *insnPtr;
            PRINT_DEBUG("result type id: %d\n", resultType);
            ++insnPtr;

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            Result_ID& result = state->resultIDs[id];
            result.type = eResultType_Variable;
            result.specType = eSpecType_LogicalPointer;

            uint32 storageClass = *insnPtr;
            PRINT_DEBUG("storage class: %d\n", storageClass);
            ++insnPtr;
            result.varDataType = state->resultIDs[resultType].specType;

            // Read optional initializer
            if (insnWordCount > 4)
            {
                uint32 init = *insnPtr;
                PRINT_DEBUG("initializer: %d\n", init);
                ++insnPtr;
            }

            PRINT_DEBUG("\n");

            break;
        }

        case 61:
        {
            PRINT_DEBUG("OpLoad\n");

            uint32 resultType = *insnPtr;
            PRINT_DEBUG("result type id: %d\n", resultType);
            ++insnPtr;

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            uint32 ptr = *insnPtr;
            PRINT_DEBUG("pointer: %d\n", ptr);
            ++insnPtr;

            for (uint16 uiWord = 0; uiWord < insnWordCount - 4; ++uiWord)
            {
                printf("Memory Operand (ignored): %d\n", *insnPtr);
                ++insnPtr;
            }

            Result_ID& result = state->resultIDs[id];
            result.type = eResultType_Data;
            result.specType = state->resultIDs[resultType].specType;

            if (!result.data)
            {
                result.elementSizeInBytes = 4; // TODO: query for this
                result.numElements = state->resultIDs[resultType].numComponents;
                result.data = malloc(result.elementSizeInBytes * result.numElements);
            }
            memcpy(result.data, state->resultIDs[ptr].data, result.elementSizeInBytes * result.numElements);

            PRINT_DEBUG("\n");

            break;
        }

        case 71:
        {
            PRINT_DEBUG("OpDecorate\n");

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            for (int16 uiWordsToProcess = insnWordCount - 2; uiWordsToProcess > 0;)
            {
                Decoration newDecoration = {};

                uint32 decorationType = *insnPtr;
                PRINT_DEBUG("Decoration type: %d\n", decorationType);
                newDecoration.type = (uint16)decorationType;
                ++insnPtr;
                --uiWordsToProcess;

                uint16 numDecorationLiterals = NumDecorationLiterals((SpecDecoration)newDecoration.type);

                for (uint16 uiLiteral = 0; uiLiteral < numDecorationLiterals; ++uiLiteral)
                {
                    uint32 decorationLiteral = *insnPtr;
                    PRINT_DEBUG("Decoration literal: %d\n", decorationLiteral);
                    newDecoration.literals[uiLiteral] = (uint16)decorationLiteral;
                    ++insnPtr;

                    --uiWordsToProcess;
                }

                state->resultIDs[id].decorations[state->resultIDs[id].numDecorations++] = newDecoration;
            }
            PRINT_DEBUG("\n");

            break;
        }

        case 248:
        {
            PRINT_DEBUG("OpLabel\n");

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            Result_ID& result = state->resultIDs[id];
            result.type = eResultType_BlockLabel;
            result.labelID = (uint16)id;

            PRINT_DEBUG("\n");

            break;
        }

        case 253:
        {
            PRINT_DEBUG("OpReturn (void)\n");

            PRINT_DEBUG("\n");

            break;
        }

        default:
        {
            PRINT_ERR("Instruction ignored: %d\n", insnOpcode);

            for (uint16 uiWord = 0; uiWord < insnWordCount - 1; ++uiWord)
            {
                word = *insnPtr;
                PRINT_ERR("Operand ignored: %d\n", word);
                ++insnPtr;
            }
            PRINT_ERR("\n");
            break;
        }
    }

    return insnPtr;
    */
}

void DestroyState_Internal(VM_Context* context, VM_State* state)
{
    free(state->results);
    free(state);
}
