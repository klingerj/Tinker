struct PushConstantData
{
  uint InstanceOffsets[4];
  // [0] is offset into bindless constant buffer for globals
  // [1] is offset into instance data uniform array
  // [2], [3] unused
};

#define PUSH_CONST_GLOBAL_INDEX 0
#define PUSH_CONST_INSTANCE_INDEX 1

[[vk::push_constant]]
PushConstantData PushConstants;

// Globals should always be available from the 0 offset.
#define GLOBAL_CONSTANT_BUFFER_OFFSET 0

struct AllGlobals
{
  float4x4 ViewProjMatrix;
  float4 CamPosition;
  uint2 Pass_ComputeCopy_dims;
  uint2 Pass_ComputeCopy_srcAndDstResIdx;
};

struct InstanceData_Basic
{
  float4x4 ModelMatrix;
};

// Keep this value up to date!
#define INSTANCE_DATA_SIZE_IN_BYTES 64;

uint CalcInstanceDataByteOffset(uint instanceID)
{
  return PushConstants.InstanceOffsets[PUSH_CONST_INSTANCE_INDEX]
         + instanceID * INSTANCE_DATA_SIZE_IN_BYTES;
}

[[vk::binding(0, 0)]] ByteAddressBuffer BindlessConstantBuffer;

//[[vk::binding(0, 1)]] ByteAddressBuffer BindlessBuffers[];
//[[vk::binding(1, 1)]] ByteAddressBuffer BindlessBuffers[];

[[vk::binding(0, 2)]] Texture2D<float4> BindlessTextures[];
[[vk::binding(0, 2)]] SamplerState
  SamplerLinearWrap; //TODO: move samplers to a different desc set entirely eventually
[[vk::binding(0, 3)]] RWTexture2D<float4> BindlessTexturesRW[];
//[[vk::binding(1, 2)]] Texture2D BindlessTexturesUint[];
//TODO: 3D textures and storage images
