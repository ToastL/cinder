#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) in vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D tex;

layout(location = 0) out vec4 outColor;

const vec3 LIGHT_DIR = vec3(-0.35, -0.87, -0.35);

void main() {
    vec3 n = normalize(fragNormal);
    float lambert = max(dot(n, -normalize(LIGHT_DIR)), 0.0);
    vec4 albedo = texture(tex, fragUV) * fragColor;
    outColor = vec4(albedo.rgb * (0.25 + 0.75 * lambert), albedo.a);
}
