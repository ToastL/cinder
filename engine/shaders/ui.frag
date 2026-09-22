#version 450

layout(set = 0, binding = 0) uniform sampler2D image;

layout(push_constant) uniform Push {
    vec2 viewport;
    float encode;
    float textGamma;
    float pixelsPerPoint;
} push;

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inBorder;
layout(location = 3) flat in vec4 inShape;
layout(location = 4) flat in vec4 inRadii;

layout(location = 0) out vec4 outColor;

const int SOLID = 0;
const int TEXTURED = 1;
const int GLYPH = 2;
const int BOX = 3;
const int OPAQUE = 4;

float roundedBox(vec2 point, vec2 extent, vec4 radii) {
    float radius = point.x < 0.0 ? (point.y < 0.0 ? radii.x : radii.w) : (point.y < 0.0 ? radii.y : radii.z);
    vec2 corner = abs(point) - extent + radius;
    return min(max(corner.x, corner.y), 0.0) + length(max(corner, 0.0)) - radius;
}

vec3 encodeSrgb(vec3 linear) {
    vec3 low = linear * 12.92;
    vec3 high = 1.055 * pow(linear, vec3(1.0 / 2.4)) - 0.055;
    return mix(low, high, step(vec3(0.0031308), linear));
}

vec4 premultiply(vec4 color) {
    return vec4(color.rgb * color.a, color.a);
}

void main() {
    vec4 sampled = texture(image, inUv);
    int mode = int(inShape.w + 0.5);
    vec4 color;

    if (mode == TEXTURED) {
        color = premultiply(sampled * inColor);
    } else if (mode == GLYPH) {
        float coverage = pow(sampled.r, push.textGamma);
        color = premultiply(vec4(inColor.rgb, inColor.a * coverage));
    } else if (mode == BOX) {
        float distance = roundedBox(inUv, inShape.xy, inRadii) * push.pixelsPerPoint;
        float border = inShape.z * push.pixelsPerPoint;
        float outer = clamp(0.5 - distance, 0.0, 1.0);
        float inner = border > 0.0 ? clamp(0.5 - distance - border, 0.0, 1.0) : outer;
        color = premultiply(inColor) * inner + premultiply(inBorder) * (outer - inner);
    } else if (mode == OPAQUE) {
        color = premultiply(vec4(sampled.rgb * inColor.rgb, inColor.a));
    } else {
        color = premultiply(inColor);
    }

    if (push.encode > 0.5 && color.a > 0.0) {
        color.rgb = encodeSrgb(color.rgb / color.a) * color.a;
    }
    outColor = color;
}
