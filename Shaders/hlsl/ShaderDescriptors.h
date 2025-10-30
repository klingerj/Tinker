#define NUM_INSTANCE_OFFSET_CONSTANTS 1
struct PushConstantData
{
  uint InstanceOffsets[NUM_INSTANCE_OFFSET_CONSTANTS];
  // [0] is offset into instance data uniform array
  // Other entries can be added to this array if needed
};

#define INSTANCE_OFFSET_INDEX_INSTANCE 0

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
  return PushConstants.InstanceOffsets[INSTANCE_OFFSET_INDEX_INSTANCE]
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
