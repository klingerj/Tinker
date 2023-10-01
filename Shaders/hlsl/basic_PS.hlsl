struct PSInput
{
    [[vk::location(0)]] float4 Position : SV_POSITION;
    [[vk::location(1)]] float2 UV       : TEXCOORD0;
    [[vk::location(2)]] float3 Normal   : NORMAL;
};

[[vk::binding(0, 3)]] Texture2D BindlessTextures[];
[[vk::binding(0, 3)]] SamplerState SamplerLinearWrap; //TODO: move samplers to a different desc set entirely eventually 

#define LIGHT_DIR normalize(float3(-1, -1, 1))
#define AMBIENT 0.05f
#define BASE_COLOR float3(0.7, 0.7, 0.7)

float4 main(PSInput Input) : SV_Target0
{
    float3 albedo = BindlessTextures[1].Sample(SamplerLinearWrap, Input.UV).rgb;
    float lambert = dot(LIGHT_DIR, normalize(Input.Normal));
    float3 finalColor = clamp(BASE_COLOR * albedo * lambert, AMBIENT, 1.0f);
    return float4(finalColor, 1.0f);
}
