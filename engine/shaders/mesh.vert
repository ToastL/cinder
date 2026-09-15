#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) in vec4 inColor;

layout(push_constant) uniform Push {
    mat4 viewProjection;
    mat4 model;
} push;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) out vec4 fragColor;

void main() {
    gl_Position = push.viewProjection * push.model * vec4(inPos, 1.0);
    fragNormal = mat3(push.model) * inNormal;
    fragUV = inUV;
    fragColor = inColor;
}
