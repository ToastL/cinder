struct Push {
    float4x4 viewProjection;
};

[[vk::push_constant]] ConstantBuffer<Push> push : register(b0);

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D tex : register(t0);
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState texSampler : register(s0);

struct Vertex {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float2 uv : TEXCOORD0;
    [[vk::location(2)]] float4 color : COLOR0;
};

struct Varyings {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
};

Varyings VSMain(Vertex input) {
    Varyings output;
    output.position = mul(push.viewProjection, float4(input.position, 1.0));
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 PSMain(Varyings input) : SV_Target0 {
    float4 color = tex.Sample(texSampler, input.uv) * input.color;
    if (color.a < 0.01) discard;
    return color;
}
