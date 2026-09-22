#version 450

layout(push_constant) uniform Push {
    vec2 viewport;
    float encode;
    float textGamma;
    float pixelsPerPoint;
} push;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec4 inBorder;
layout(location = 4) in vec4 inShape;
layout(location = 5) in vec4 inRadii;

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec4 outBorder;
layout(location = 3) flat out vec4 outShape;
layout(location = 4) flat out vec4 outRadii;

void main() {
    gl_Position = vec4(inPosition / push.viewport * 2.0 - 1.0, 0.0, 1.0);
    outUv = inUv;
    outColor = inColor;
    outBorder = inBorder;
    outShape = inShape;
    outRadii = inRadii;
}
