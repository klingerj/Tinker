#include "ShaderDescriptors.h"

struct PSInput
{
    [[vk::location(0)]] float4 Position : SV_POSITION;
    [[vk::location(1)]] float2 UV       : TEXCOORD0;
    [[vk::location(2)]] float3 Normal   : NORMAL;
};

#define LIGHT_DIR normalize(float3(-1, -1, 1))
#define AMBIENT 0.05f
#define BASE_COLOR float3(0.7, 0.7, 0.7)

float4 main(PSInput Input) : SV_Target0
{
    float3 albedo0 = BindlessTextures[0].Sample(SamplerLinearWrap, Input.UV).rgb;
    float3 albedo1 = BindlessTextures[1].Sample(SamplerLinearWrap, Input.UV).rgb;
    float3 albedo = lerp(albedo0, albedo1, Input.Normal.x * 0.5 + 0.5);
    float lambert = dot(LIGHT_DIR, normalize(Input.Normal));
    float3 finalColor = clamp(BASE_COLOR * albedo * abs(lambert), AMBIENT, 1.0f);
    return float4(finalColor, 1.0f);
}
