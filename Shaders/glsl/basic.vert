#version 450

layout(set = 0, binding = 0) uniform DescriptorInstanceData
{
    mat4 modelMatrix;
} InstanceData;

layout(location = 0) in vec4 vertPosition;

void main() {
    gl_Position = InstanceData.modelMatrix * vec4(vertPosition.xyz, 1.0);
}
