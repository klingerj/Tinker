#version 450

layout(location = 0) in vec3 normal;
layout(location = 0) out vec4 outColor;

#define LIGHT_DIR normalize(vec3(1, 1, 1))

void main() {
    float lambert = dot(LIGHT_DIR, normal);
    outColor = vec4(vec3(lambert), 1.0);
}
