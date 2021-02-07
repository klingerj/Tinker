#ifndef VM_STATE_H
#define VM_STATE_H

#include "KHR/spirv.h"
#include "KHR/GLSL.std.450.h"
#include "CoreDefines.h"

struct VM_Shader;
typedef uint16 ResultID;
struct VM_State
{
    // Shader this state was created from
    const VM_Shader* ownerShader;

    // Result ID data
    uint32 numResultIDs;
    ResultID* resultIDs;
    
    // TODO: store function stack data

    void Init(const VM_Shader* shader)
    {
        ownerShader = shader;
        numResultIDs = 0;
        resultIDs = nullptr;
    }
};
VM_State* CreateState_Internal(const VM_Shader* shader);
void DestroyState_Internal(VM_State* state);
//-----

// State data types
#define MAX_DECORATIONS 8
struct Decoration
{
    uint16 type;
    uint16 literals[2];
};

enum ResultType : uint8
{
    eResultType_EntryPoint,
    eResultType_Type,
    eResultType_Constant,
    eResultType_Variable,
    eResultType_Function,
    eResultType_BlockLabel,
    eResultType_Max
};

enum ResultDataType : uint8
{
    eResultDataType_Boolean,
    eResultDataType_Int,
    eResultDataType_Float,
    eResultDataType_Pointer,
    eResultDataType_Vector,
    eResultDataType_Matrix,
    eResultDataType_Array,
    eResultDataType_Struct,
    eResultDataType_Void,
    eResultDataType_Max
};

struct TypeData
{
    ResultID* memberTypes; // member types - are result IDs
    uint16* memberSizesInBytes; // size of each member in bytes
    ResultDataType type; // float, vector, pointer
    uint16 numElements;
};

// TODO: dynamically allocate this?
#define MAX_FUNC_PARAMETERS 8
struct FunctionTypeData
{
    ResultDataType parameters[MAX_FUNC_PARAMETERS];
    ResultDataType returnType;
    uint8 numParameters;
};

struct MemberList
{
    union Member
    {
        uint8 u8;
        float f32;
        double f64;
        uint32 u32;
        int32 i32;
        uint64 u64;
    } *members; // actual list of member data

    // TODO: not sure if we need this yet:
    //MemberList* subMemberList; // for vector/array/matrix/struct
};

struct Result
{
    // Type / Function
    union
    {
        TypeData typeData;
        FunctionTypeData funcTypeData;
    } type;

    union
    {
        // Constant/Variable
        struct
        {
            MemberList memberList;
        };

        // Function
        struct
        {
            uint32 insnStartOffset; // offset into instruction stream
            ResultID funcTypeID;
        };

        // Block label
        struct
        {
            ResultID labelID;
        };
    } data;

    // TODO: dynamically allocate decorations?
    Decoration decorations[MAX_DECORATIONS];
    uint8 numDecorations = 0;

    // enum for the union
    ResultType resultType;
};

#endif
