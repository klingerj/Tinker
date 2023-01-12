struct PushConstantData
{
    float2 Scale;
    float2 Translate;
};

[[vk::push_constant]]
PushConstantData PushConstants;

struct DescGlobal
{
    float4x4 ViewProjMatrix;
};

#define PositionDataType float2
#define UVDataType float2
#define ColorDataType uint

[[vk::binding(0, 1)]] StructuredBuffer<PositionDataType> PositionData;
[[vk::binding(1, 1)]] StructuredBuffer<UVDataType>       UVData;
[[vk::binding(2, 1)]] StructuredBuffer<ColorDataType>    ColorData;

struct VSOutput
{
    [[vk::location(0)]] float4 Position : SV_POSITION;
    [[vk::location(1)]] float2 UV       : TEXCOORD0;
    [[vk::location(2)]] float4 Color    : COLOR;
};

VSOutput main(uint VertexIndex : SV_VertexID, uint InstanceIndex : SV_InstanceID)
{
    float2 ModelPos = PositionData.Load(VertexIndex).xy;
    float2 UV = UVData.Load(VertexIndex);
    uint Color = ColorData.Load(VertexIndex);
    float4 ColorUnpacked;
    ColorUnpacked.r = Color & 0xF;
    ColorUnpacked.g = (Color >> 8) & 0xF;
    ColorUnpacked.b = (Color >> 16) & 0xF;
    ColorUnpacked.a = (Color >> 24) & 0xF;

    VSOutput Out;
    Out.Position = float4(ModelPos * PushConstants.Scale + PushConstants.Translate, 0, 1);
    Out.UV = UV;
    Out.Color = ColorUnpacked;
    return Out;
}
