#version 450

layout(set = 1, binding = 0) readonly buffer DescMesh_Position
{
    vec4 Buffer[];
} PositionData;

layout(set = 1, binding = 1) readonly buffer DescMesh_UV
{
    vec2 Buffer[];
} UVData;

layout(set = 1, binding = 2) readonly buffer DescMesh_Normal
{
    vec4 Buffer[];
} NormalData;

layout(location = 0) out vec2 outUV;

void main()
{
    vec4 modelPos = vec4(PositionData.Buffer[gl_VertexIndex].xyz, 1.0);
    gl_Position = modelPos;
    vec2 uv = modelPos.xy * 0.5 + 0.5;
    uv.y = 1 - uv.y;
    outUV = uv;
}
