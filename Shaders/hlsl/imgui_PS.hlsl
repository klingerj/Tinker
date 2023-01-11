[[vk::binding(0, 0)]] Texture2D SrcImage;
[[vk::binding(0, 0)]] SamplerState SamplerLinearWrap;

struct PSInput
{
    [[vk::location(0)]] float4 Position : SV_POSITION;
    [[vk::location(1)]] float2 UV       : TEXCOORD0;
    [[vk::location(2)]] float3 Color    : COLOR;
};

float4 main(PSInput Input) : SV_Target0
{
    return float4(Input.Color * SrcImage.SampleLevel(SamplerLinearWrap, Input.UV, 0).rgb, 1.0f);
}
