[[vk::binding(0, 0)]] Texture2D SrcColorImage;
[[vk::binding(1, 0)]] RWTexture2D<float4> DstImage;

[numthreads(16, 16, 0)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint2 Coord = DispatchThreadID.xy;

    if (Coord.x >= 800 || Coord.y >= 600)
    {
        return;
    }

    float3 Color = float3(1, 0, 1);// SrcColorImage[Coord].rgb;
    float Grayscale = Color.r; // todo do the dot product
    DstImage[Coord].rgb = Grayscale.rrr;
}
