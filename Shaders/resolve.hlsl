struct Push {
    float exposure;
};

[[vk::push_constant]] ConstantBuffer<Push> push : register(b0);

struct Varyings {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
};

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D scene : register(t0);
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState sceneSampler : register(s0);

static const float SHOULDER = 0.75;
static const float VIGNETTE = 0.25;

Varyings VSMain(uint vertex : SV_VertexID) {
    Varyings output;
    output.uv = float2((vertex << 1) & 2, vertex & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

float3 tonemap(float3 x) {
    float range = 1.0 - SHOULDER;
    float3 over = max(x - SHOULDER, 0.0);
    return min(x, SHOULDER) + range * (1.0 - exp(-over / range));
}

float4 PSMain(Varyings input) : SV_Target0 {
    float3 color = tonemap(scene.Sample(sceneSampler, input.uv).rgb * push.exposure);
    float2 centered = input.uv - 0.5;
    color *= saturate(1.0 - dot(centered, centered) * VIGNETTE);
    return float4(color, 1.0);
}
