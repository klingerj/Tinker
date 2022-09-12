
// TODO: figure out push constants
[[vk::push_constant]]
cbuffer PushConstant
{
    uint InstanceData[4];
    // [0] is offset into instance data uniform array
    // [1], [2], [3] unused
} PushConstants;

struct DescGlobal
{
    float4x4 ViewProjMatrix;
};

#define MAX_INSTANCES // TODO: use an unbounded array
struct Instance_Data
{
    float4x4 ModelMatrix;
};

struct DescInstance
{
    Instance_Data data[MAX_INSTANCES];
};

struct DescMesh_Position
{
    float4 Data[];
};

struct DescMesh_UV
{
    float2 Data[];
};

struct DescMesh_Normal
{
    float4 Data;
};

[[vk::binding(0, 0)]] cbuffer GlobalData   { DescGlobal GlobalData; };
[[vk::binding(0, 1)]] cbuffer InstanceData { DescInstance InstanceData; };
[[vk::binding(0, 2)]] cbuffer PositionData { DescMesh_Position PositionData; };
[[vk::binding(1, 2)]] cbuffer UVData       { DescMesh_UV UVData; };
[[vk::binding(2, 2)]] cbuffer NormalData   { DescMesh_Normal NormalData; };

struct VSOutput
{
    [[vk::location(0)]] float2 outUV;
    [[vk::location(1)]] float3 outNormal;
};

VSOutput main(uint VertexIndex : SV_VertexID, uint InstanceIndex : SV_InstanceID)
{
    VSOutput Out = (VSOutput)0;

    float4x4 ViewProjMat = GlobalData.GlobalData.ViewProjMatrix;
    float4x4 ModelMat = InstanceData.InstanceData.data[SV_InstanceID + PushConstants.InstanceData[0]].ModelMatrix;

    return Out;
}