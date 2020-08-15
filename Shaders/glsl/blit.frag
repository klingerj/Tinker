#version 450

layout(binding = 0) uniform sampler2D srcImage;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(texture(srcImage, texCoord).rgb, 1.0);
}
