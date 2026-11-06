struct Varyings {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
};

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D scene : register(t0);
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState sceneSampler : register(s0);

Varyings VSMain(uint vertex : SV_VertexID) {
    Varyings output;
    output.uv = float2((vertex << 1) & 2, vertex & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

float4 PSMain(Varyings input) : SV_Target0 {
    return scene.Sample(sceneSampler, input.uv);
}
