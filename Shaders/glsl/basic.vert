#version 450

layout(location = 0) in vec4 vertPosition;

void main() {
    gl_Position = vec4(vertPosition.xyz, 1.0);
}
