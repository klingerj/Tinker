#include "ShaderDescriptors.h"

#define PositionDataType float4
#define UVDataType float2
#define NormalDataType float4

[[vk::binding(0, 1)]] StructuredBuffer<PositionDataType> PositionData;
[[vk::binding(1, 1)]] StructuredBuffer<UVDataType>       UVData;
[[vk::binding(2, 1)]] StructuredBuffer<NormalDataType>   NormalData;

struct VSOutput
{
    [[vk::location(0)]] float4 Position : SV_POSITION;
    [[vk::location(1)]] float2 UV       : TEXCOORD0;
    [[vk::location(2)]] float3 Normal   : NORMAL;
    [[vk::location(3)]] uint InstanceIndex : INSTANCEID;
    [[vk::location(4)]] float3 WorldPos : WORLDPOS;
};

VSOutput main(uint VertexIndex : SV_VertexID, uint InstanceIndex : SV_InstanceID)
{
    float4x4 ViewProjMat = BindlessConstantBuffer.Load<AllGlobals>(PushConstants.InstanceOffsets[0]).ViewProjMatrix;
    float4x4 ModelMat = BindlessConstantBuffer.Load<InstanceData_Basic>(PushConstants.InstanceOffsets[1] + InstanceIndex * 64).ModelMatrix;
    
    float4 ModelPos = float4(PositionData.Load(VertexIndex).xyz, 1.0f);
    float2 UV = UVData.Load(VertexIndex);
    float3 Normal = normalize(NormalData.Load(VertexIndex).xyz);

    VSOutput Out;
    Out.Position = mul(ViewProjMat, mul(ModelMat, ModelPos));
    Out.UV = UV;
    Out.Normal = normalize(ModelPos.xyz);// Normal;
    Out.WorldPos = mul(ModelMat, ModelPos).xyz;
    Out.InstanceIndex = InstanceIndex;
    return Out;
}
