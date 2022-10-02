
struct PSInput
{
    [[vk::location(0)]] float4 Position  : SV_POSITION;
    [[vk::location(1)]] float2 inUV     : TEXCOORD0;
    [[vk::location(2)]] float3 inNormal : NORMAL;
};

struct PSOutput
{
    [[vk::location(0)]] float4 outColor;
};

#define LIGHT_DIR normalize(float3(-1, -1, 1))
#define AMBIENT 0.05f
#define BASE_COLOR float3(0.7, 0.7, 0.7)

float4 main(PSInput Input) : SV_Target0
{
    float lambert = dot(LIGHT_DIR, normalize(Input.inNormal));
    float3 finalColor = clamp(BASE_COLOR * lambert, AMBIENT, 1.0f);
    
    float2 UV = Input.inUV;
    float4 InPos = Input.Position;

    //PSOutput Out;
    //Out.outColor = float4(finalColor, 1.0f);
    //return Out;
    return float4(finalColor, 1.0f);
}
