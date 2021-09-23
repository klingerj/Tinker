#version 450

layout(set = 0, binding = 0) uniform DescGlobal
{
    mat4 viewProjMatrix;
} GlobalData;

layout(set = 1, binding = 0) readonly buffer DescMesh_Position
{
    vec4 Buffer[];
} PositionData;

layout(location = 0) out vec2 outUV;

void main()
{
    vec4 modelPos = vec4(PositionData.Buffer[gl_VertexIndex].xyz, 1.0);
    mat4 viewProjMat = GlobalData.viewProjMatrix;
    gl_Position = viewProjMat * modelPos;
    outUV = vec2(gl_VertexIndex %2, gl_VertexIndex / 2);
}
