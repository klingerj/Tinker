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
};

struct InstanceData_Basic
{
    float4x4 ModelMatrix;
};

[[vk::binding(0, 0)]] ByteAddressBuffer BindlessConstantBuffer;
