#ifndef SPIRV_VM_H
#define SPIRV_VM_H

#include "VMContext.h"
#include "VMState.h"
#include "VMShader.h"

// API
inline VM_Context* CreateContext()
{
    return CreateContext_Internal();
}
inline void DestroyContext(VM_Context* context)
{
    DestroyContext_Internal(context);
}
inline VM_Shader* CreateShader(VM_Context* context, const uint32* spvFile, uint32 fileSizeInBytes)
{
    return CreateShader_Internal(context, spvFile, fileSizeInBytes);
}
inline void DestroyShader(VM_Context* context, VM_Shader* shader)
{
    DestroyShader_Internal(context, shader);
}
inline VM_State* CreateState(VM_Context* context, VM_Shader* shader)
{
    return CreateState_Internal(context, shader);
}
inline void DestroyState(VM_Context* context, VM_State* state)
{
    DestroyState_Internal(context, state);
}

/*struct Descriptors
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
};*/

/*
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
*/

//------------------------------------------------------------
/*
struct Result_ID
{
    ResultType type;
    uint8 numDecorations = 0;
    Decoration decorations[MAX_DECORATIONS] = {};

    union
    {
        // eResultType_EntryPoint
        //struct
        //{
        //    uint16 executionModel;
        //};

        // eResultType_Type
        struct
        {
            SpecType specType;

            // Union of various spec type metadata
            union
            {
                // Void type has no metadata

                // Float type
                struct
                {
                    uint8 bitWidth;
                };

                // Pointer type
                struct
                {
                    uint16 storageClass;
                    uint8 pointerSpecType;
                };

                // Vector type
                struct
                {
                    uint8 numComponents;
                    uint16 componentSpecType;
                };

                // Function type
                struct
                {
                    SpecType returnType;
                    uint8 numParameters;
                    SpecType parameters[MAX_FUNC_PARAMETERS];
                };
            };
        };

        // eResultType_Data
        struct
        {
            uint16 elementSizeInBytes;
            uint16 numElements;
            void* data;
        };

        // eResultType_Variable
        struct
        {
            uint16 varDataType;
            void* varData;
        };

        // eResultType_Function
        struct
        {
            uint32 insnStartOffset;
            uint16 funcTypeID;
        };

        // eResultType_BlockLabel
        struct
        {
            uint16 labelID;
        };
    };
};

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
*/
#endif