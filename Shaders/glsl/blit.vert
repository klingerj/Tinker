#version 450

layout(location = 0) in vec4 vertPosition;
layout(location = 0) out vec2 texCoord;

void main() {
    gl_Position = vec4(vertPosition.xyz, 1.0);
    texCoord = vertPosition.xy * 0.5 + 0.5;
}
