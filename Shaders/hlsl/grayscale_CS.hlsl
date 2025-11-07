#include "ShaderDescriptors.h"

// TODO: this is temp until all buffers are moved over to bindless! just delete later
[[vk::binding(0, 1)]] ByteAddressBuffer BindlessConstantBuffer2;
// ---------------

[numthreads(16, 16, 1)] void main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
  uint2 Coord = DispatchThreadID.xy;
  uint2 Dims = BindlessConstantBuffer.Load<AllGlobals>(GLOBAL_CONSTANT_BUFFER_OFFSET)
                 .Pass_ComputeCopy_dims;
  const uint2 resIndices =
    BindlessConstantBuffer.Load<AllGlobals>(GLOBAL_CONSTANT_BUFFER_OFFSET)
      .Pass_ComputeCopy_srcAndDstResIdx;

  if (Coord.x >= Dims.x || Coord.y >= Dims.y)
  {
    return;
  }

  const uint srcResIdx = resIndices.x;
  const uint dstResIdx = resIndices.y;

  float3 Color = BindlessTexturesRW[srcResIdx][Coord].rgb;
  float Grayscale = Color.r;                            // todo do the dot product
  BindlessTexturesRW[dstResIdx][Coord].rgb = Color.rgb; // Grayscale.rrr;
}
