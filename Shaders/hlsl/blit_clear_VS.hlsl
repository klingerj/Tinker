#define PositionDataType float4

[[vk::binding(0, 0)]] StructuredBuffer<PositionDataType> PositionData;

struct VSOutput
{
    [[vk::location(0)]] float4 Position : SV_POSITION;
};

VSOutput main(uint VertexIndex : SV_VertexID)
{
    float4 ModelPos = float4(PositionData.Load(VertexIndex).xyz, 1.0f);

    VSOutput Out;
    Out.Position = ModelPos;
    return Out;
}
