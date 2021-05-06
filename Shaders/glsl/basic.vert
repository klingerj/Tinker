#version 450

layout(set = 0, binding = 0) uniform DescGlobal
{
    mat4 viewProjMatrix;
} GlobalData;

layout(set = 1, binding = 0) uniform DescInstance
{
    mat4 modelMatrix;
} InstanceData;

layout(location = 0) in vec4 vertPosition;
layout(location = 1) in vec2 vertUV;
layout(location = 2) in vec3 vertNormal;
layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outNormal;

void main()
{
    gl_Position = GlobalData.viewProjMatrix * InstanceData.modelMatrix * vec4(vertPosition.xyz, 1.0);
    outNormal = (transpose(inverse(InstanceData.modelMatrix)) * vec4(vertNormal, 0)).xyz;
    outUV = vertUV;
}
