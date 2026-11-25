struct Push {
    float4x4 reproject;
    float4 params;
};

[[vk::push_constant]] ConstantBuffer<Push> push : register(b0);

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D fresh : register(t0);
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState freshSampler : register(s0);
[[vk::combinedImageSampler]] [[vk::binding(1, 1)]] Texture2D history : register(t1);
[[vk::combinedImageSampler]] [[vk::binding(1, 1)]] SamplerState historySampler : register(s1);

struct Varyings {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
};

Varyings VSMain(uint vertex : SV_VertexID) {
    Varyings output;
    output.uv = float2((vertex << 1) & 2, vertex & 2);
    output.position = float4(output.uv * 2.0 - 1.0, 0.0, 1.0);
    return output;
}

int2 frameOffset() {
    int index = int(push.params.y) % 4;
    return int2(index == 1 || index == 2 ? 1 : 0, index == 1 || index == 3 ? 1 : 0);
}

float4 freshAt(int2 texel, int2 freshSize) {
    int2 clamped = clamp(texel, int2(0, 0), freshSize - 1);
    return fresh.SampleLevel(freshSampler, (float2(clamped) + 0.5) / float2(freshSize), 0.0);
}

float4 PSMain(Varyings input) : SV_Target0 {
    uint lowWidth, lowHeight, freshWidth, freshHeight;
    history.GetDimensions(lowWidth, lowHeight);
    fresh.GetDimensions(freshWidth, freshHeight);
    int2 lowSize = int2(lowWidth, lowHeight);
    int2 freshSize = int2(freshWidth, freshHeight);

    int2 pixel = min(int2(floor(input.uv * float2(lowSize))), lowSize - 1);
    int2 relative = pixel - frameOffset();
    int2 nearest = int2(floor(float2(relative) * 0.5 + 0.5));
    if (all((relative & 1) == 0)) return freshAt(relative / 2, freshSize);

    float4 low = float4(1e9, 1e9, 1e9, 1e9);
    float4 high = float4(-1e9, -1e9, -1e9, -1e9);
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            float4 neighbour = freshAt(nearest + int2(x, y), freshSize);
            low = min(low, neighbour);
            high = max(high, neighbour);
        }
    }

    if (push.params.z < 0.5) return freshAt(nearest, freshSize);

    float4 previous = mul(push.reproject, float4(input.uv * 2.0 - 1.0, push.params.x, 1.0));
    float2 previousUv = previous.xy / previous.w * 0.5 + 0.5;
    if (previous.w <= 0.0 || any(previousUv < 0.0) || any(previousUv > 1.0)) return freshAt(nearest, freshSize);

    return clamp(history.SampleLevel(historySampler, previousUv, 0.0), low, high);
}
