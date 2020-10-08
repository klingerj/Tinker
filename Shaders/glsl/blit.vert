#version 450

layout(location = 0) in vec4 vertPosition;
layout(location = 1) in vec3 vertNormal; // not used, TODO: remove
layout(location = 0) out vec2 texCoord;

void main()
{
    gl_Position = vec4(vertPosition.xyz, 1.0);
    vec2 uv = vertPosition.xy * 0.5 + 0.5;
    uv.y = 1 - uv.y;
    texCoord = uv;
}
