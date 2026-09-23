struct Push {
    float4x4 viewProjection;
    float4x4 model;
};

[[vk::push_constant]] Push push;

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D tex;
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState texSampler;

struct Vertex {
    [[vk::location(0)]] float3 position : POSITION;
    [[vk::location(1)]] float3 normal : NORMAL;
    [[vk::location(2)]] float2 uv : TEXCOORD0;
    [[vk::location(3)]] float4 color : COLOR0;
};

struct Varyings {
    float4 position : SV_Position;
    [[vk::location(0)]] float3 normal : NORMAL;
    [[vk::location(1)]] float2 uv : TEXCOORD0;
    [[vk::location(2)]] float4 color : COLOR0;
};

static const float3 LIGHT_DIR = float3(-0.35, -0.87, -0.35);

Varyings VSMain(Vertex input) {
    Varyings output;
    output.position = mul(push.viewProjection, mul(push.model, float4(input.position, 1.0)));
    output.normal = mul((float3x3)push.model, input.normal);
    output.uv = input.uv;
    output.color = input.color;
    return output;
}

float4 PSMain(Varyings input) : SV_Target0 {
    float3 n = normalize(input.normal);
    float lambert = max(dot(n, -normalize(LIGHT_DIR)), 0.0);
    float4 albedo = tex.Sample(texSampler, input.uv) * input.color;
    return float4(albedo.rgb * (0.25 + 0.75 * lambert), albedo.a);
}
