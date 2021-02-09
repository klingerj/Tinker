#include "VMShader.h"

VM_Shader* CreateShader_Internal(VM_Context* context, const uint32* spvFile, uint32 fileSizeInBytes)
{
    const uint32* spvFileBase = spvFile;
    const uint32* spvFilePtr = spvFileBase;

    // Header
    uint32 magicNumber = ReadSpirvWord(&spvFilePtr);
    PRINT_DEBUG("Magic number: %x\n", magicNumber);

    uint32 versionBytes = ReadSpirvWord(&spvFilePtr);
    uint8 major = (uint8)((versionBytes & 0x00FF0000) >> 16);
    uint8 minor = (uint8)((versionBytes & 0x0000FF00) >> 8);
    PRINT_DEBUG("Major version number: %d\n", major);
    PRINT_DEBUG("Minor version number: %d\n", minor);

    uint32 generatorMagicNumber = ReadSpirvWord(&spvFilePtr);
    generatorMagicNumber >>= 16; // want upper 16 bits for generator number
    PRINT_DEBUG("Generator magic number: %d\n", generatorMagicNumber);

    if (generatorMagicNumber == 8)
    {
        PRINT_DEBUG("Generator: Khronos's Glslang Reference Front End.\n");
    }
    else
    {
        // TODO: print message for other spv generators?
    }

    uint32 bound = ReadSpirvWord(&spvFilePtr); // max id number
    PRINT_DEBUG("Bound number: %d\n", bound);
    ++spvFilePtr;

    // Skip next word
    // "Reserved for instruction scheme, if needed"
    CONSUME_SPIRV_WORD(&spvFilePtr);

    VM_Shader* newShader = (VM_Shader*)malloc(sizeof(VM_Shader));
    newShader->ownerContext = context;
    newShader->boundNum = bound;
    newShader->numCapabilities = 0;
    const uint32 headerSizeInWords = 5;
    newShader->insnStreamSizeInWords = fileSizeInBytes / sizeof(uint32) - headerSizeInWords;
    newShader->insnStream = (uint32*)malloc(sizeof(uint32) * newShader->insnStreamSizeInWords);
    uint32 insnStreamSizeInBytes = newShader->insnStreamSizeInWords * sizeof(uint32);
    memcpy_s(newShader->insnStream, insnStreamSizeInBytes, spvFileBase + headerSizeInWords, insnStreamSizeInBytes);
    return newShader;
}
