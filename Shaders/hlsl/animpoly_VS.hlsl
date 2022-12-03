struct DescGlobal
{
    float4x4 ViewProjMatrix;
};

#define PositionDataType float4
#define UVDataType float2
#define NormalDataType float4

[[vk::binding(0, 0)]] cbuffer Global { DescGlobal GlobalData; };
[[vk::binding(0, 1)]] StructuredBuffer<PositionDataType> PositionData;

struct VSOutput
{
    [[vk::location(0)]] float4 Position : SV_POSITION;
    [[vk::location(1)]] float4 Color    : COLOR;
};

VSOutput main(uint VertexIndex : SV_VertexID)
{
    float4x4 ViewProjMat = GlobalData.ViewProjMatrix;
    float4 ModelPos = float4(PositionData.Load(VertexIndex).xyz, 1.0f);

    VSOutput Out;
    Out.Position = mul(ViewProjMat, ModelPos);
    Out.Color = float4(abs(ModelPos.xyz), 1);
    return Out;
}
