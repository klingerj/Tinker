#version 450

layout(set = 0, binding = 0) uniform DescriptorInstanceData
{
    mat4 modelMatrix;
    mat4 viewProjMatrix;
} InstanceData;

layout(location = 0) in vec4 vertPosition;
layout(location = 1) in vec3 vertNormal;
layout(location = 0) out vec3 normal;

void main()
{
    gl_Position = InstanceData.viewProjMatrix * InstanceData.modelMatrix * vec4(vertPosition.xyz, 1.0);
    normal = (transpose(inverse(InstanceData.modelMatrix)) * vec4(vertNormal, 0)).xyz;
}
