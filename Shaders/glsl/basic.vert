#version 450

layout (push_constant) uniform PushConstant {
    uint instanceData[4];
    // [0] is offset into instance data uniform array
    // [1], [2], [3] unused
} pushConstants;

layout(set = 0, binding = 0) uniform DescGlobal
{
    mat4 viewProjMatrix;
} GlobalData;

// NOTE: These definitions must match the definitions in the engine.
#define MAX_INSTANCES 128
struct Instance_Data
{
    mat4 modelMatrix;
};
layout(set = 1, binding = 0) uniform DescInstance
{
    Instance_Data data[MAX_INSTANCES];
} InstanceData;

layout(set = 2, binding = 0) readonly buffer DescMesh_Position
{
    vec4 Buffer[];
} PositionData;

layout(set = 2, binding = 1) readonly buffer DescMesh_UV
{
    vec2 Buffer[];
} UVData;

layout(set = 2, binding = 2) readonly buffer DescMesh_Normal
{
    vec4 Buffer[];
} NormalData;

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;

void main()
{
    mat4 modelMat = InstanceData.data[gl_InstanceIndex + pushConstants.instanceData[0]].modelMatrix;
    mat4 viewProjMat = GlobalData.viewProjMatrix;

    vec4 modelPos = vec4(PositionData.Buffer[gl_VertexIndex].xyz, 1.0);
    vec2 uv = UVData.Buffer[gl_VertexIndex];
    vec3 normal = normalize(NormalData.Buffer[gl_VertexIndex].xyz);

    gl_Position = (viewProjMat * modelMat) * modelPos;
    outNormal = normalize(transpose(inverse(modelMat)) * vec4(normal, 0)).xyz;
    outUV = uv;
}
