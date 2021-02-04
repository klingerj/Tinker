#pragma once

#define DISABLE_MEM_TRACKING
#include "../Include/Core/CoreDefines.h"

#include <stdio.h>
#include <cstring>

namespace SPIRV_VM
{

#define ENABLE_SHADER_PARSING_LOGGING_ALL

#if defined(ENABLE_SHADER_PARSING_LOGGING_ALL)
#define ENABLE_SHADER_PARSING_LOGGING_ERRORS
#define ENABLE_SHADER_PARSING_LOGGING_DEBUG
#endif

#ifdef ENABLE_SHADER_PARSING_LOGGING_ERRORS
#define PRINT_ERR(...) printf(__VA_ARGS__);
#else PRINT_ERR(...)
#define 
#endif

#ifdef ENABLE_SHADER_PARSING_LOGGING_DEBUG
#define PRINT_DEBUG(...) printf(__VA_ARGS__);
#else
#define PRINT_DEBUG(...)
#endif

// SPIR-V spec values

// https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#_types
enum SpecType : uint8
{
    eSpecType_Boolean = 0,
    eSpecType_Integer = 1,
    eSpecType_Float = 2,
    eSpecType_Numerical = 3,
    eSpecType_Scalar = 4,
    eSpecType_Vector = 5,
    eSpecType_Matrix = 6,
    eSpecType_Array = 7,
    eSpecType_Structure = 8,
    eSpecType_Aggregate = 9,
    eSpecType_Composite = 10,
    eSpecType_Image = 11,
    eSpecType_Sampler = 12,
    eSpecType_SampledImage = 13,
    eSpecType_PhysicalPointer = 14,
    eSpecType_LogicalPointer = 15,
    eSpecType_Concrete = 16,
    eSpecType_Abstract = 17,
    eSpecType_Opaque = 18,
    eSpecType_VariablePointer = 19,
    eSpecType_Void = 20,
    eSpecType_Max
};

enum SpecDecorations : uint16
{
    eSpecDecoration_RelaxedPrecision = 0,
    // TODO: add all the spec decorations
    eSpecDecoration_Max
};

#define MAX_CAPABILITIES 64
// NOTE: these must match the SPIR-V spec
enum Capability : uint16
{
    eCapability_Matrix = 0,
    eCapability_Shader,
    // TODO: support more capabilities
    eCapability_Max
};
//-----

// Helpers

void PrintInstructionName(uint16 opcode)
{
    PRINT_DEBUG("Opcode: %d - ", opcode);

    switch (opcode)
    {
        case 3:
        {
            PRINT_DEBUG("OpSource\n");
            break;
        }

        case 5:
        {
            PRINT_DEBUG("OpName\n");
            break;
        }

        case 11:
        {
            PRINT_DEBUG("OpExtInstImport\n");
            break;
        }

        case 14:
        {
            PRINT_DEBUG("OpMemoryModel\n");
            break;
        }

        case 15:
        {
            PRINT_DEBUG("OpEntryPoint\n");
            break;
        }

        case 16:
        {
            PRINT_DEBUG("OpExecutionMode\n");
            break;
        }

        case 17:
        {
            PRINT_DEBUG("OpCapability\n");
            break;
        }

        case 19:
        {
            PRINT_DEBUG("OpTypeVoid\n");
            break;
        }

        case 22:
        {
            PRINT_DEBUG("OpTypeFloat\n");
            break;
        }
        
        case 23:
        {
            PRINT_DEBUG("OpTypeVector\n");
            break;
        }

        case 32:
        {
            PRINT_DEBUG("OpTypePointer\n");
            break;
        }

        case 33:
        {
            PRINT_DEBUG("OpTypeFunction\n");
            break;;
        }

        case 43:
        {
            PRINT_DEBUG("OpConstant\n");
            break;
        }

        case 44:
        {
            PRINT_DEBUG("OpConstantComposite\n");
            break;
        }

        case 71:
        {
            PRINT_DEBUG("OpDecorate\n");
            break;
        }

        default:
        {
            PRINT_ERR("Unknown operation: %d\n", opcode);
            break;
        }
    }
}

//-----

struct VM_Shader
{
    uint16 capabilities[MAX_CAPABILITIES];
    uint16 numCapabilities;
    uint16 executionModel;

    // TODO: support multiple entry points and an exec mode for each
    uint16 entryPointID;
    uint16 entryPointExecMode;
    uint8 entryPointName[4];

    // insn id max value, also equal to number of elements allocated in resultIDs
    uint32 boundNum;
    uint32 insnStreamSizeInWords;
    uint32* insnStream;
};

VM_Shader CreateShader(const uint32* spvFile, uint32 fileSizeInBytes)
{
    const uint32* spvFileBase = spvFile;
    const uint32* spvFilePtr = spvFileBase;

    // Header
    {
        uint32 magicNumber = *spvFilePtr;
        PRINT_DEBUG("Magic number: %x\n", magicNumber);
        ++spvFilePtr;

        uint32 versionBytes = *spvFilePtr;
        uint8 major = (uint8)((versionBytes & 0x00FF0000) >> 16);
        uint8 minor = (uint8)((versionBytes & 0x0000FF00) >> 8);
        PRINT_DEBUG("Major version number: %d\n", major);
        PRINT_DEBUG("Minor version number: %d\n", minor);
        ++spvFilePtr;

        uint32 generatorMagicNumber = *spvFilePtr;
        generatorMagicNumber >>= 16; // want upper 16 bits for generator number
        PRINT_DEBUG("Generator magic number: %d\n", generatorMagicNumber);
        ++spvFilePtr;

        if (generatorMagicNumber == 8)
        {
            PRINT_DEBUG("Generator: Khronos's Glslang Reference Front End.\n");
        }
        else
        {
            // TODO: print message for other spv generators?
        }
    }

    uint32 bound = *spvFilePtr; // max id number
    PRINT_DEBUG("Bound number: %d\n\n", bound)
    ++spvFilePtr;

    // Skip next word
    // "Reserved for instruction scheme, if needed"
    ++spvFilePtr;

    VM_Shader newShader = {};
    newShader.boundNum = bound;
    newShader.numCapabilities = 0;
    newShader.insnStreamSizeInWords = 0;

    // Process instructions up through and including "execution-mode declarations"
    // https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#_a_id_logicallayout_a_logical_layout_of_a_module
    uint32 wordsRead = (uint32)(spvFilePtr - spvFileBase);
    while ((wordsRead * 4) < fileSizeInBytes)
    {
        uint32 opcodeWord = *spvFilePtr;
        uint16 insnWordCount = (uint16)((opcodeWord & 0xFFFF0000) >> 16);
        uint16 insnOpcode = (uint16)(opcodeWord & 0x0000FFFF);
        ++spvFilePtr;

        // Process instructions in the file
        uint32 word;

        bool stopProcessingShader = false;
        switch (insnOpcode)
        {
            case 3:
            {
                PrintInstructionName(insnOpcode);

                word = *spvFilePtr;
                PRINT_DEBUG("source language: %d\n", word);
                ++spvFilePtr;

                word = *spvFilePtr;
                PRINT_DEBUG("version: %d\n", word);
                ++spvFilePtr;

                if (insnWordCount > 3)
                {
                    PRINT_DEBUG("file/source:\n");

                    // NOTE: this print segment is currently untested
                    for (uint16 uiWord = 2; uiWord < insnWordCount - 1; ++uiWord)
                    {
                        word = *spvFilePtr;
                        uint8* chars = (uint8*)&word;
                        PRINT_DEBUG("%c", chars[0]);
                        PRINT_DEBUG("%c", chars[1]);
                        PRINT_DEBUG("%c", chars[2]);
                        PRINT_DEBUG("%c", chars[3]);
                        ++spvFilePtr;
                    }
                    PRINT_DEBUG("\n");
                }
                break;
            }

            case 11:
            {
                PrintInstructionName(insnOpcode);

                word = *spvFilePtr;
                PRINT_DEBUG("id: %d\n", word);
                ++spvFilePtr;

                PRINT_DEBUG("name: ");
                for (uint16 uiWord = 1; uiWord < insnWordCount - 1; ++uiWord)
                {
                    word = *spvFilePtr;
                    uint8* chars = (uint8*)&word;
                    PRINT_DEBUG("%c", chars[0]);
                    PRINT_DEBUG("%c", chars[1]);
                    PRINT_DEBUG("%c", chars[2]);
                    PRINT_DEBUG("%c", chars[3]);
                    ++spvFilePtr;
                }
                PRINT_DEBUG("\n");
                break;
            }

            case 14:
            {
                PrintInstructionName(insnOpcode);

                word = *spvFilePtr;
                PRINT_DEBUG("addressing model: %d\n", word);
                ++spvFilePtr;

                word = *spvFilePtr;
                PRINT_DEBUG("memory model: %d\n", word);
                ++spvFilePtr;

                break;
            }

            case 15:
            {
                PrintInstructionName(insnOpcode);

                word = *spvFilePtr;
                PRINT_DEBUG("execution model: %d\n", word);
                newShader.executionModel = (uint16)word;
                ++spvFilePtr;

                word = *spvFilePtr;
                PRINT_DEBUG("entry point id: %d\n", word);
                newShader.entryPointID = (uint16)word;
                ++spvFilePtr;

                word = *spvFilePtr;
                PRINT_DEBUG("literal name: ");
                memcpy(newShader.entryPointName, (uint8*)&word, 4);
                uint8* chars = (uint8*)&word;
                PRINT_DEBUG("%c", chars[0]);
                PRINT_DEBUG("%c", chars[1]);
                PRINT_DEBUG("%c", chars[2]);
                PRINT_DEBUG("%c", chars[3]);
                PRINT_DEBUG("\n");
                ++spvFilePtr;

                for (uint16 uiWord = 3; uiWord < insnWordCount - 1; ++uiWord)
                {
                    word = *spvFilePtr;
                    PRINT_DEBUG("interface operand: %d\n", word);
                    ++spvFilePtr;
                }

                break;
            }

            case 16:
            {
                PrintInstructionName(insnOpcode);

                word = *spvFilePtr;
                PRINT_DEBUG("entry point: %d\n", word);
                ++spvFilePtr;

                word = *spvFilePtr;
                PRINT_DEBUG("execution mode: %d\n", word);
                uint16 execMode = (uint16)word;
                newShader.entryPointExecMode = execMode;
                ++spvFilePtr;

                for (uint16 uiWord = 2; uiWord < insnWordCount - 1; ++uiWord)
                {
                    word = *spvFilePtr;
                    PRINT_DEBUG("execution mode literal: %d\n", word);
                    ++spvFilePtr;
                }

                break;
            }

            case 17:
            {
                PrintInstructionName(insnOpcode);

                word = *spvFilePtr;
                PRINT_DEBUG("Capability: %d\n", word);
                
                // Add capability to list of supported capabilities
                uint16 capability = (uint16)word;
                newShader.capabilities[newShader.numCapabilities++] = capability;
                ++spvFilePtr;

                break;
            }

            default:
            {
                // All other instructions are to be executed at runtime
                stopProcessingShader = true;
                break;
            }
        }

        PRINT_DEBUG("\n");
        if (stopProcessingShader) break;

        wordsRead = (uint32)(spvFilePtr - spvFileBase);
    }

    // the file contents after reading early shader instructions, in words
    newShader.insnStreamSizeInWords = fileSizeInBytes / 4 - wordsRead;
    newShader.insnStream = new uint32[newShader.insnStreamSizeInWords];
    memcpy(newShader.insnStream, spvFileBase + wordsRead, newShader.insnStreamSizeInWords * 4);
    return newShader;
}

void DestroyShader(const VM_Shader& shader)
{
    if (shader.insnStream)
    {
        delete shader.insnStream;
    }
}

struct Descriptors
{
    // TODO: uniform data
    struct
    {
        float someData[4];
    } descriptorSets[3];

    void Init()
    {
        descriptorSets[0] = {};
        descriptorSets[1] = {};
        descriptorSets[2] = {};
    }

    bool operator==(const Descriptors& other) const
    {
        bool result = true;

        for (uint32 i = 0; i < 3; ++i)
        {
            result &= descriptorSets[i].someData[0] == other.descriptorSets[i].someData[0];
            result &= descriptorSets[i].someData[1] == other.descriptorSets[i].someData[1];
            result &= descriptorSets[i].someData[2] == other.descriptorSets[i].someData[2];
            result &= descriptorSets[i].someData[3] == other.descriptorSets[i].someData[3];
        }

        return result;
    }
    bool operator!=(const Descriptors& other) const
    {
        return !(*this == other);
    }
};

#define MAX_DECORATIONS 8
struct Decoration
{
    uint16 type;
    uint16 literal0;
    uint16 literal1;
};

enum ResultType : uint8
{
    //eResultType_EntryPoint = 0,
    eResultType_Type = 0,
    eResultType_Max
};

struct Result_ID
{
    ResultType type;
    uint8 numDecorations = 0;
    Decoration decorations[MAX_DECORATIONS] = {};

    union
    {
        // eResultType_EntryPoint
        /*struct
        {
            uint16 executionModel;
        };*/

        // eResultType_Type
        struct
        {
            SpecType specType;
        };
    };
};

struct State
{
    uint32 programCounter;
    //TODO: remove these
    uint64 regA;
    uint64 regB;

    uint32 numResultIDs;
    Result_ID* resultIDs;

    void Init()
    {
        programCounter = 0;
        regA = 0;
        regB = 0;
        numResultIDs = 0;
        resultIDs = nullptr;
    }

    bool operator==(const State& other) const
    {
        return (programCounter == other.programCounter &&
            regA == other.regA &&
            regB == other.regB);
    }
    bool operator!=(const State& other) const
    {
        return !(*this == other);
    }
};

template <typename T>
struct State_Log
{
    uint32 nextStateIndex;
    T previousStates[128 + 1]; // TODO: needs to be dynamically allocated probs

    void Init()
    {
        nextStateIndex = 0;
        for (uint16 i = 0; i < 128 + 1; ++i)
        {
            previousStates[i].Init();
        }
    }
};
typedef SPIRV_VM::State_Log<SPIRV_VM::State> StateLog;
typedef SPIRV_VM::State_Log<SPIRV_VM::Descriptors> DescLog;

template <typename T>
void SaveStateInLog(State_Log<T>* log, T* state)
{
    log->previousStates[log->nextStateIndex++] = *state;
}

template <typename T>
void RestoreLastStateFromLog(State_Log<T>* log, T* state)
{
    memcpy(state, &log->previousStates[--log->nextStateIndex], sizeof(T));
}

const uint32* ExecuteSingleInsn(State* state, Descriptors* descriptors, const uint32* insnStream)
{
    // TODO: execute shader instructions
    const uint32* insnPtr = insnStream;

    uint32 word = *insnPtr;
    uint16 insnWordCount = (uint16)((word & 0xFFFF0000) >> 16);
    uint16 insnOpcode = (uint16)(word & 0x0000FFFF);
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

        case 71:
        {
            PRINT_DEBUG("OpDecorate\n");

            uint32 id = *insnPtr;
            PRINT_DEBUG("id: %d\n", id);
            ++insnPtr;

            for (int16 uiWordsToProcess = insnWordCount - 1; uiWordsToProcess > 0;)
            {
                Decoration newDecoration = {};

                uint32 decorationType = *insnPtr;
                PRINT_DEBUG("Decoration type: %d\n", decorationType);
                newDecoration.type = (uint16)decorationType;
                ++insnPtr;

                switch (decorationType)
                {
                    // One-literal decorations
                    case 30:
                    {
                        uint32 decorationLiteral = *insnPtr;
                        PRINT_DEBUG("Decoration literal: %d\n", decorationLiteral);
                        newDecoration.literal0 = (uint16)decorationLiteral;
                        ++insnPtr;

                        uiWordsToProcess -= 3;
                        break;
                    }

                    // Two-literal decorations
                    case 2:
                    {
                        uint32 decorationLiteral = *insnPtr;
                        PRINT_DEBUG("Decoration literal: %d\n", decorationLiteral);
                        newDecoration.literal0 = (uint16)decorationLiteral;
                        ++insnPtr;

                        decorationLiteral = *insnPtr;
                        PRINT_DEBUG("Decoration literal: %d\n", decorationLiteral);
                        newDecoration.literal1 = (uint16)decorationLiteral;
                        ++insnPtr;

                        uiWordsToProcess -= 4;
                        break;
                    }

                    // Zero-literal decorations
                    default:
                    {
                        uiWordsToProcess -= 2;
                        break;
                    }
                }

                state->resultIDs[id].decorations[state->resultIDs[id].numDecorations++] = newDecoration;
            }
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

    state->regA++;
    state->regB++;
    descriptors->descriptorSets[0].someData[0] += 2.1f;
    return insnPtr;
}

void StepForwardShader(State* state, Descriptors* descriptors, VM_Shader* shader)
{
    ExecuteSingleInsn(state, descriptors, &shader->insnStream[state->programCounter]);
    ++state->programCounter;
}

void ExecuteEntireShader(State* state, Descriptors* descriptors, const VM_Shader* shader)
{
    state->numResultIDs = shader->boundNum;
    state->resultIDs = new Result_ID[state->numResultIDs];
    // TODO: free this memory, and probably need to make a function to create a state from a shader

    const uint32* insnStart = shader->insnStream;

    for (; (uint32)(insnStart - shader->insnStream) < shader->insnStreamSizeInWords;)
    {
        const uint32* nextInsn = ExecuteSingleInsn(state, descriptors, insnStart);
        insnStart = nextInsn;
        ++state->programCounter; // TODO: remove this?
    }
}

}
