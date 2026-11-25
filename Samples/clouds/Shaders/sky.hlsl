struct Push {
    float4x4 invViewProj;
    float4 sunDir;
};

[[vk::push_constant]] ConstantBuffer<Push> push : register(b0);

struct Varyings {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 ndc : NDC;
};

Varyings VSMain(uint vertex : SV_VertexID) {
    Varyings output;
    float2 uv = float2((vertex << 1) & 2, vertex & 2);
    float2 clip = uv * 2.0 - 1.0;
    output.position = float4(clip, 1.0, 1.0);
    output.ndc = clip;
    return output;
}

float3 worldRay(float2 ndc) {
    float4 nearPoint = mul(push.invViewProj, float4(ndc, 0.0, 1.0));
    float4 farPoint = mul(push.invViewProj, float4(ndc, 1.0, 1.0));
    return normalize(farPoint.xyz / farPoint.w - nearPoint.xyz / nearPoint.w);
}

float4 PSMain(Varyings input) : SV_Target0 {
    float3 rayDir = worldRay(input.ndc);
    float3 sunDir = normalize(push.sunDir.xyz);

    static const float3 ZENITH = float3(0.08, 0.28, 0.68);
    static const float3 HORIZON = float3(0.78, 0.88, 0.96);

    float t = saturate(rayDir.y);
    float3 sky = lerp(HORIZON, ZENITH, smoothstep(0.0, 0.75, t));

    float d = max(dot(rayDir, sunDir), 0.0);
    sky += float3(1.4, 1.2, 0.7) * pow(d, 8.0) * 0.5;
    sky += float3(2.0, 1.6, 1.1) * pow(d, 256.0);
    sky += float3(3.0, 2.4, 1.6) * pow(d, 4096.0);

    return float4(sky, 1.0);
}
