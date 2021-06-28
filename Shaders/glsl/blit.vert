#version 450

layout(set = 1, binding = 0) readonly buffer DescMesh_Position
{
    vec4 Buffer[];
} PositionData;

layout(location = 0) out vec2 outUV;

void main()
{
    vec4 modelPos = vec4(PositionData.Buffer[gl_VertexIndex].xyz, 1.0);
    gl_Position = modelPos;
    vec2 uv = modelPos.xy * 0.5 + 0.5;
    uv.y = 1 - uv.y;
    outUV = uv;
}
