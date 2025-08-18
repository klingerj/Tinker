#include "ShaderDescriptors.h"

// TODO: this is temp until all buffers are moved over to bindless! just delete later
[[vk::binding(0, 1)]] ByteAddressBuffer BindlessConstantBuffer2;
// ---------------

[numthreads(16, 16, 1)]
void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    uint2 Coord = DispatchThreadID.xy;
    Material_ComputeCopyImage2D Constants = BindlessConstantBuffer.Load<Material_ComputeCopyImage2D>(64 + 16 /*PushConstants.InstanceOffsets[2]*/);

    if (Coord.x >= Constants.dims.x || Coord.y >= Constants.dims.y)
    {
        return;
    }

    float3 Color = BindlessTexturesRW[Constants.srcIndexBindless][Coord].rgb;
    float Grayscale = Color.r; // todo do the dot product
    BindlessTexturesRW[Constants.dstIndexBindless][Coord].rgb = Color.rgb;// Grayscale.rrr;
}
