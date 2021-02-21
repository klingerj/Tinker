#ifndef VM_SHADER_H
#define VM_SHADER_H

#include "CoreDefines.h"
#include "VMContext.h"
#include "VMTypes.h"

#include <string.h>

#define MAX_CAPABILITIES 64
typedef struct vm_shader
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
} VM_Shader;

VM_Shader* CreateShader_Internal(VM_Context* context, const uint32* spvFile, uint32 fileSizeInBytes);

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
