#include "CoreDefines.h"
#include "GLSL.std.450_Ops.h"
#include "VMState.h"

#include <math.h>

GLSL_STD_450_OP_HANDLER(Glsl_std_450_FClamp)
{
    uint32 xID = ReadSpirvWord(insnStreamPtr);
    uint32 minValID = ReadSpirvWord(insnStreamPtr);
    uint32 maxValID = ReadSpirvWord(insnStreamPtr);

    Result* result = &(state->results[resultID]);
    result->data.constantVariableData.typeResultID = (ResultID)resultTypeID;
    
    Result* typeResult = &(state->results[resultTypeID]);

    MemberList* memberList = &result->data.constantVariableData.memberList;
    if (!memberList->members)
    {
        PRINT_DEBUG("No member data already allocated for dst ID in Glsl_std_450_FClamp.\n");
        memberList->members = (Member*)malloc(sizeof(Member));
    }
    //-----

    Result* xResult = &(state->results[xID]);
    Result* minResult = &(state->results[minValID]);
    Result* maxResult = &(state->results[maxValID]);

    if (typeResult->type.typeData.typeSizeInBytes > 4)
    {
        // double precision
        memberList->members[0].f64 =
            fmin(fmax(xResult->data.constantVariableData.memberList.members->f64,
                      minResult->data.constantVariableData.memberList.members->f64),
                      maxResult->data.constantVariableData.memberList.members->f64);
    }
    else
    {
        memberList->members[0].f32 =
            (float)fmin(fmax(xResult->data.constantVariableData.memberList.members->f32,
                             minResult->data.constantVariableData.memberList.members->f32),
                             maxResult->data.constantVariableData.memberList.members->f32);
    }
}
