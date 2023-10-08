[[vk::binding(0, 0)]] RWTexture2D<float4> SrcColorImage;
[[vk::binding(1, 0)]] RWTexture2D<float4> DstImage;

[numthreads(16, 16, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint2 Coord = DispatchThreadID.xy;

    if (Coord.x >= 800 || Coord.y >= 600)
    {
        return;
    }

    float3 Color = SrcColorImage[Coord].rgb;
    float Grayscale = Color.r; // todo do the dot product
    DstImage[Coord].rgb = Color.rgb;// Grayscale.rrr;
}
