#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec4 inColor;

layout(push_constant) uniform Push {
    mat4 viewProjection;
} push;

layout(location = 0) out vec2 fragUV;
layout(location = 1) out vec4 fragColor;

void main() {
    gl_Position = push.viewProjection * vec4(inPos, 1.0);
    fragUV = inUV;
    fragColor = inColor;
}
