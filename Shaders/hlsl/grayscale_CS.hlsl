[[vk::binding(0, 0)]] Texture2D SrcColorImage;
[[vk::binding(1, 0)]] RWTexture2D<float4> DstImage;

[numthreads(32, 0, 0)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint2 Coord = DispatchThreadID.xy;
    float3 Color = SrcColorImage[Coord].rgb;
    float Grayscale = Color.r; // todo do the dot product
    DstImage[Coord].rgb = Grayscale.rrr;
}
