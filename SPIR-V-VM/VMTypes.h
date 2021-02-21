#ifndef VM_TYPES_H
#define VM_TYPES_H

#include "KHR/spirv.h"
#include "KHR/GLSL.std.450.h"

#define CONSUME_SPIRV_WORD(insnStream) (++(*insnStream))
inline uint32 ReadSpirvWord(const uint32** insnStream)
{
    uint32 word = **insnStream;
    CONSUME_SPIRV_WORD(insnStream);
    return word;
}
inline void ConsumeSpirvWords(const uint32** insnStream, uint16 numWords)
{
    for (uint16 uiWord = 0; uiWord < numWords; ++uiWord)
    {
        CONSUME_SPIRV_WORD(insnStream);
    }
}

#define WORD_COUNT(word) ((word & ~SpvOpCodeMask) >> SpvWordCountShift)
#define OPCODE(word) (word & SpvOpCodeMask)
//-----

// State data types
#define MAX_DECORATIONS 8
typedef struct decor
{
    uint16 type;
    uint16 literals[2];
} Decoration;

typedef enum result_data_type
{
    eResultDataType_Invalid = 0,
    eResultDataType_Boolean,
    eResultDataType_Int,
    eResultDataType_Float,
    eResultDataType_Pointer,
    eResultDataType_Vector,
    eResultDataType_Matrix,
    eResultDataType_Array,
    eResultDataType_Struct,
    eResultDataType_Void,
} ResultDataType;
typedef uint16 ResultID;

/*typedef struct type_data
{
    struct // TODO: change me
    {
        // Aggregate member data
        ResultID* memberTypes; // member types - are result IDs
        uint32* memberSizesInBytes; // size of each member in bytes

        // Scalar data
        ResultID elementType;
        uint32 elementSizeInBytes;
    } memberData;
    ResultDataType resultType; // float, vector, pointer
    uint16 numElements;
} TypeData;
*/

typedef struct type_data
{
    ResultID* subTypes; // sub-type IDs - e.g., pointed type if this type is a pointer
    uint32 typeSizeInBytes; // size of an element of this type in bytes
    ResultDataType resultType; // the actual data type of this result
    uint16 numElements; // number of elements
    //uint16 numSubTypes; // number of sub-type IDs
} TypeData;

/*inline uint8 IsScalarType(ResultDataType type)
{
    return (
        type == eResultDataType_Boolean ||
        type == eResultDataType_Int ||
        type == eResultDataType_Float
        );
}*/

// TODO: dynamically allocate this?
#define MAX_FUNC_PARAMETERS 8
typedef struct function_type_data
{
    ResultDataType parameters[MAX_FUNC_PARAMETERS];
    ResultDataType returnType;
    uint8 numParameters;
} FunctionTypeData;

typedef union
{
    uint8 u8;
    float f32;
    double f64;
    uint32 u32;
    int32 i32;
    uint64 u64;
} Member;

typedef struct
{
    Member* members; // actual list of member data

    // TODO: not sure if we need this yet:
    //MemberList* subMemberList; // for vector/array/matrix/struct
} MemberList;

typedef enum result_type
{
    eResultType_EntryPoint,
    eResultType_Type,
    eResultType_Constant,
    eResultType_Variable,
    eResultType_Function,
    eResultType_BlockLabel,
    eResultType_Max
} ResultType;

typedef struct result
{
    char* name;
    uint16 nameLen;

    // Type / Function
    union type_function_data
    {
        TypeData typeData;
        FunctionTypeData funcTypeData;
    } type;

    union
    {
        // Constant/Variable
        struct constant_variable
        {
            MemberList memberList;
            ResultID typeResultID;
            uint16 storageClass;
        } constantVariableData;

        // Function / Entry point
        struct function_entry_point
        {
            // Function
            const uint32* insnPtr; // pointer into instruction stream
            ResultID funcReturnTypeID;
            ResultID funcTypeID;

            // Entry point
            uint16 executionModel;
            uint16 executionMode;
        } functionEntryPointData;

        // Block label
        struct block_label
        {
            const uint32* insnBlockPtr; // pointer into instruction stream
        } BlockLabelData;

    } data;

    // TODO: dynamically allocate decorations?
    Decoration decorations[MAX_DECORATIONS];
    uint8 numDecorations;

    // enum for the union
    ResultType resultType;
} Result;
//-----

// Helper functions

inline uint16 CountChars(const uint32* insnStream)
{
    const char* charsBase = (const char*)insnStream;
    const char* chars = charsBase;
    while (*chars != '\0')
    {
        ++chars;
    }
    return (uint16)(chars - charsBase) + 1;
}

inline uint16 NumDecorationLiterals(uint16 decorationID)
{
    switch (decorationID)
    {
        case SpvDecorationSpecId:
        case SpvDecorationArrayStride:
        case SpvDecorationMatrixStride:
        case SpvDecorationBuiltIn:
        case SpvDecorationUniformId:
        case SpvDecorationStream:
        case SpvDecorationLocation:
        case SpvDecorationComponent:
        case SpvDecorationIndex:
        case SpvDecorationBinding:
        case SpvDecorationDescriptorSet:
        case SpvDecorationXfbBuffer:
        case SpvDecorationXfbStride:
        case SpvDecorationFuncParamAttr:
        case SpvDecorationFPRoundingMode:
        case SpvDecorationFPFastMathMode:
        case SpvDecorationInputAttachmentIndex:
        case SpvDecorationAlignment:
        case SpvDecorationMaxByteOffset:
        case SpvDecorationAlignmentId:
        case SpvDecorationMaxByteOffsetId:
        {
            return 1;
        }

        case SpvDecorationLinkageAttributes:
        case SpvDecorationMergeINTEL:
        {
            return 2;
        }

        default:
        {
            return 0;
        }
    }
}

#endif
