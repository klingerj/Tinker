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

layout(location = 0) in vec4 vertPosition;
layout(location = 1) in vec2 vertUV;
layout(location = 2) in vec3 vertNormal;
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;

void main()
{
    mat4 modelMat = InstanceData.data[gl_InstanceIndex + pushConstants.instanceData[0]].modelMatrix;
    gl_Position = (GlobalData.viewProjMatrix * modelMat) * vec4(vertPosition.xyz, 1.0);
    outNormal = normalize(transpose(inverse(modelMat)) * vec4(normalize(vertNormal), 0)).xyz;
    outUV = vertUV;
}
