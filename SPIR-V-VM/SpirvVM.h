#ifndef SPIRV_VM_H
#define SPIRV_VM_H

#include "CoreDefines.h"

struct vm_context;
struct vm_shader;
struct vm_state;
typedef struct vm_context VM_Context;
typedef struct vm_shader VM_Shader;
typedef struct vm_state VM_State;

// API
VM_Context* CreateContext();
void DestroyContext(VM_Context* context);

VM_Shader* CreateShader(VM_Context* context, const uint32* spvFile, uint32 fileSizeInBytes);
void DestroyShader(VM_Context* context, VM_Shader* shader);

VM_State* CreateState(VM_Context* context, VM_Shader* shader);
void DestroyState(VM_Context* context, VM_State* state);

uint8 CallEntryPointByName(VM_Context* context, VM_State* state, const char* name);
void AddStateInputData(VM_Context* context, VM_State* state, uint32 location, void* data, uint32 dataSizeInBytes);

#endif
