#ifndef VM_SHADER_H
#define VM_SHADER_H

#include "KHR/spirv.h"
#include "KHR/GLSL.std.450.h"
#include "CoreDefines.h"
#include "VMContext.h"

#include <cstring>

#define MAX_CAPABILITIES 64
struct VM_Shader
{
    VM_Context* ownerContext;

    uint16 capabilities[MAX_CAPABILITIES];
    uint16 numCapabilities;
    uint16 addressingModel;
    uint16 memoryModel;

    // insn id max value, also equal to number of elements allocated in resultIDs
    uint32 boundNum;
    uint32 insnStreamSizeInWords;
    uint32* insnStream;
};

inline VM_Shader* CreateShader_Internal(VM_Context* context, const uint32* spvFile, uint32 fileSizeInBytes)
{
    const uint32* spvFileBase = spvFile;
    const uint32* spvFilePtr = spvFileBase;

    // Header
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

    uint32 bound = *spvFilePtr; // max id number
    PRINT_DEBUG("Bound number: %d\n", bound);
    ++spvFilePtr;

    // Skip next word
    // "Reserved for instruction scheme, if needed"
    ++spvFilePtr;

    VM_Shader* newShader = (VM_Shader*)malloc(sizeof(VM_Shader));
    newShader->ownerContext = context;
    newShader->boundNum = bound;
    newShader->numCapabilities = 0;
    const uint32 headerSizeInWords = 5;
    newShader->insnStreamSizeInWords = fileSizeInBytes/sizeof(uint32) - headerSizeInWords;
    newShader->insnStream = (uint32*)malloc(sizeof(uint32) * newShader->insnStreamSizeInWords);
    memcpy(newShader->insnStream, spvFileBase + headerSizeInWords, newShader->insnStreamSizeInWords * sizeof(uint32));
    return newShader;

    // Process instructions up through and including "execution-mode declarations"
    // https://www.khronos.org/registry/spir-v/specs/unified1/SPIRV.html#_a_id_logicallayout_a_logical_layout_of_a_module
    /*uint32 wordsRead = (uint32)(spvFilePtr - spvFileBase);
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
    }*/
}

inline void DestroyShader_Internal(VM_Context* context, VM_Shader* shader)
{
    if (shader->insnStream)
    {
        free(shader->insnStream);
    }

    if (shader)
    {
        free(shader);
    }
}

#endif
