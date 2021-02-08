#ifndef VM_TYPES_H
#define VM_TYPES_H

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

typedef uint16 ResultID;
struct TypeData
{
    union
    {
        // Member data
        struct
        {
            ResultID* memberTypes; // member types - are result IDs
            uint16* memberSizesInBytes; // size of each member in bytes
        };

        // Scalar size
        uint16 elementSizeInBytes;
    } memberData;
    ResultDataType resultType; // float, vector, pointer
    uint16 numElements;
};

inline bool IsScalarType(ResultDataType type)
{
    return (
        type == eResultDataType_Boolean ||
        type == eResultDataType_Int ||
        type == eResultDataType_Float
        );
}

// TODO: dynamically allocate this?
#define MAX_FUNC_PARAMETERS 8
struct FunctionTypeData
{
    ResultDataType parameters[MAX_FUNC_PARAMETERS];
    ResultDataType returnType;
    uint8 numParameters;
};

union Member
{
    uint8 u8;
    float f32;
    double f64;
    uint32 u32;
    int32 i32;
    uint64 u64;
};

struct MemberList
{
    Member* members; // actual list of member data

    // TODO: not sure if we need this yet:
    //MemberList* subMemberList; // for vector/array/matrix/struct
};

struct Result
{
    char* name;
    uint16 nameLen;

    // Type / Function
    union
    {
        TypeData typeData;
        FunctionTypeData funcTypeData;
    } type;

    union
    {
        // Entry point
        /*struct
        {
            uint16 executionModel;
            char name[4];
        };*/

        // Constant/Variable
        struct
        {
            MemberList memberList;
        };

        // Function / Entry point
        struct
        {
            uint32 insnStartOffset; // offset into instruction stream
            ResultID funcTypeID;

            ResultID labelID; // block label

            uint16 executionModel;
            uint16 executionMode;
        };

        // Block label
        /*struct
        {
            ResultID labelID;
        };*/

    } data;

    // TODO: dynamically allocate decorations?
    Decoration decorations[MAX_DECORATIONS];
    uint8 numDecorations = 0;

    // enum for the union
    ResultType resultType;
};

#endif
