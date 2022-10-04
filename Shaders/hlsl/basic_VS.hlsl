struct PushConstantData
{
    uint InstanceOffsets[4];
    // [0] is offset into instance data uniform array
    // [1], [2], [3] unused
};

[[vk::push_constant]]
PushConstantData PushConstants;

struct DescGlobal
{
    float4x4 ViewProjMatrix;
};

#define MAX_INSTANCES 128 // TODO: use an unbounded array
struct Instance_Data
{
    float4x4 ModelMatrix;
};

struct DescInstance
{
    Instance_Data data[MAX_INSTANCES];
};

#define PositionDataType float4
#define UVDataType float2
#define NormalDataType float4

[[vk::binding(0, 0)]] cbuffer Global   { DescGlobal GlobalData; };
[[vk::binding(0, 1)]] cbuffer Instance { DescInstance InstanceData; };
[[vk::binding(0, 2)]] StructuredBuffer<PositionDataType> PositionData;
[[vk::binding(1, 2)]] StructuredBuffer<UVDataType>       UVData;
[[vk::binding(2, 2)]] StructuredBuffer<NormalDataType>   NormalData;

struct VSOutput
{
    [[vk::location(0)]] float4 Position : SV_POSITION;
    [[vk::location(1)]] float2 UV       : TEXCOORD0;
    [[vk::location(2)]] float3 Normal   : NORMAL;
};

VSOutput main(uint VertexIndex : SV_VertexID, uint InstanceIndex : SV_InstanceID)
{
    float4x4 ViewProjMat = GlobalData.ViewProjMatrix;
    float4x4 ModelMat = InstanceData.data[InstanceIndex + PushConstants.InstanceOffsets[0]].ModelMatrix;

    float4 ModelPos = float4(PositionData.Load(VertexIndex).xyz, 1.0f);
    float2 UV = UVData.Load(VertexIndex);
    float3 Normal = normalize(NormalData.Load(VertexIndex).xyz);

    VSOutput Out;
    Out.Position = mul(ViewProjMat, mul(ModelMat, ModelPos));
    Out.UV = UV;
    Out.Normal = Normal;
    return Out;
}
