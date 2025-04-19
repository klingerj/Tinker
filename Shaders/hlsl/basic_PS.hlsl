#include "ShaderDescriptors.h"

struct PSInput
{
    [[vk::location(0)]] float4 Position : SV_POSITION;
    [[vk::location(1)]] float2 UV       : TEXCOORD0;
    [[vk::location(2)]] float3 Normal   : NORMAL;
    [[vk::location(3)]] uint InstanceIndex : INSTANCEID;
    [[vk::location(4)]] float3 WorldPos : WORLDPOS;
};

#define LIGHT_DIR normalize(float3(1, -1, -1))
#define LIGHT_INTENSITY 1.0
#define M_PI 3.14159265358979323846
#define ONE_OVER_PI (1.0 / M_PI)

float3 Schlick(float3 f0, float3 f90, float vDotH)
{
    return f0 + (f90 - f0) * pow(clamp(1.0 - vDotH, 0.0, 1.0), 5.0);
}

float Schlick(float f0, float f90, float vDotH)
{
    float x = clamp(1.0 - vDotH, 0.0, 1.0);
    float x2 = x * x;
    float x5 = x * x2 * x2;
    return f0 + (f90 - f0) * x5;
}

float V_GGX(float NdotL, float NdotV, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;

    float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);
    float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - alphaRoughnessSq) + alphaRoughnessSq);

    float GGX = GGXV + GGXL;
    if (GGX > 0.0)
    {
        return 0.5 / GGX;
    }
    return 0.0;
}

float D_GGX(float NdotH, float alphaRoughness)
{
    float alphaRoughnessSq = alphaRoughness * alphaRoughness;
    float f = (NdotH * NdotH) * (alphaRoughnessSq - 1.0) + 1.0;
    return alphaRoughnessSq / (M_PI * f * f);
}

float3 specGGX(float alphaRoughness, float NdotL, float NdotV, float NdotH)
{
    float Vis = V_GGX(NdotL, NdotV, alphaRoughness);
    float D = D_GGX(NdotH, alphaRoughness);

    return float3(Vis * D, Vis * D, Vis * D);
}

float4 main(PSInput Input) : SV_Target0
{
    //float3 albedo0 = BindlessTextures[0].Sample(SamplerLinearWrap, Input.UV).rgb;
    //float3 albedo1 = BindlessTextures[1].Sample(SamplerLinearWrap, Input.UV).rgb;
    //float3 albedo = lerp(albedo0, albedo1, Input.Normal.x * 0.5 + 0.5);
    const float roughness = (Input.InstanceIndex / 8) / 7.0;
    const float metallic = (Input.InstanceIndex % 8) / 7.0;
    
    float3 albedo = float3(1, 1, 1);
    float3 c_diff = albedo;// lerp(albedo, float3(0, 0, 0), metallic);
    float3 l = -LIGHT_DIR;
    float3 v = normalize(BindlessConstantBuffer.Load<AllGlobals>(PushConstants.InstanceOffsets[0]).CamPosition.xyz - Input.WorldPos);
    float3 h = normalize(l + v);

    float nDotL = saturate(dot(Input.Normal, l)); // Lambert's law
    float nDotV = saturate(dot(Input.Normal, v));
    float nDotH = saturate(dot(Input.Normal, h));
    float absVdotH = abs(dot(v, h));
    float3 brdfDiffuse = c_diff * ONE_OVER_PI; // energy conservation 
    float3 diffuseTerm = brdfDiffuse * LIGHT_INTENSITY * nDotL;
    
    // Fresnel terms
    float3 fresnelDielectric = 0.04 + (1 - 0.04) * pow((1 - absVdotH), 5.0);//Schlick(0.04, 1.0, absVdotH);
    float3 fresnelMetal = albedo + (float3(1, 1, 1) - albedo) * pow((1 - absVdotH), 5.0); //Schlick(c_diff, float3(1.0, 1.0, 1.0), absVdotH);

    // Assume non-aniso material for now 
    float3 brdfSpec = LIGHT_INTENSITY * nDotL * specGGX(roughness * roughness, nDotL, nDotV, nDotH);

    float3 brdfMetal = brdfSpec * fresnelMetal;
    float3 brdfDielectric = lerp(diffuseTerm, brdfSpec, fresnelDielectric);

    float3 finalColor = lerp(brdfDielectric, brdfMetal, metallic);
    return float4(finalColor, 1.0f);
}
