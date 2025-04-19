struct PushConstantData
{
    uint InstanceOffsets[4];
    // [0] is offset into bindless constant buffer for globals
    // [1] is offset into instance data uniform array
    // [2], [3] unused
};

[[vk::push_constant]]
PushConstantData PushConstants;

struct AllGlobals
{
    float4x4 ViewProjMatrix;
    float4 CamPosition;
};

struct InstanceData_Basic
{
    float4x4 ModelMatrix;
};

struct Material_ComputeCopyImage2D
{
    uint srcIndexBindless;
    uint dstIndexBindless;
    uint2 dims;
};

[[vk::binding(0, 0)]] ByteAddressBuffer BindlessConstantBuffer;

//[[vk::binding(0, 1)]] ByteAddressBuffer BindlessBuffers[];
//[[vk::binding(1, 1)]] ByteAddressBuffer BindlessBuffers[];

[[vk::binding(0, 2)]] Texture2D<float4> BindlessTextures[];
[[vk::binding(0, 2)]] SamplerState SamplerLinearWrap; //TODO: move samplers to a different desc set entirely eventually 
[[vk::binding(0, 3)]] RWTexture2D<float4> BindlessTexturesRW[];
//[[vk::binding(1, 2)]] Texture2D BindlessTexturesUint[];
//TODO: 3D textures and storage images 
