#version 450

layout(set = 0, binding = 0) uniform DescriptorInstanceData
{
    mat4 modelMatrix;
    mat4 viewProjMatrix;
} InstanceData;

layout(location = 0) in vec4 vertPosition;

void main() {
    gl_Position = InstanceData.viewProjMatrix * InstanceData.modelMatrix * vec4(vertPosition.xyz, 1.0);
}
