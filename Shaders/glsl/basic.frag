#version 450

layout(location = 0) in vec3 normal;
layout(location = 0) out vec4 outColor;

#define LIGHT_DIR normalize(vec3(1, 1, 1))
#define AMBIENT 0.05
#define BASE_COLOR vec3(0.7, 0.7, 0.7)

void main()
{
    float lambert = dot(LIGHT_DIR, normal);
    vec3 finalColor = clamp(BASE_COLOR * lambert, AMBIENT, 1.0);
    outColor = vec4(finalColor, 1.0);
}
