struct Push {
    float4x4 invViewProj;
    float4x4 invWorld;
    float4 cameraTime;   // xyz=camera, w=time
    float4 sunCoverage;  // xyz=sunDir, w=coverage
    float4 extras;       // x=density, y=windSpeed
};

[[vk::push_constant]] ConstantBuffer<Push> push : register(b0);

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D sceneDepth : register(t0);
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState depthSampler : register(s0);

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

float3 unprojectWorld(float2 ndc, float depth) {
    float4 h = mul(push.invViewProj, float4(ndc, depth, 1.0));
    return h.xyz / h.w;
}

float3 worldRay(float2 ndc) {
    float3 nearPoint = unprojectWorld(ndc, 0.0);
    float3 farPoint = unprojectWorld(ndc, 1.0);
    return normalize(farPoint - nearPoint);
}

float hash31(float3 p) {
    p = frac(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return frac((p.x + p.y) * p.z);
}

float valueNoise(float3 p) {
    float3 i = floor(p);
    float3 f = frac(p);
    f = f * f * (3.0 - 2.0 * f);

    float n000 = hash31(i + float3(0, 0, 0));
    float n100 = hash31(i + float3(1, 0, 0));
    float n010 = hash31(i + float3(0, 1, 0));
    float n110 = hash31(i + float3(1, 1, 0));
    float n001 = hash31(i + float3(0, 0, 1));
    float n101 = hash31(i + float3(1, 0, 1));
    float n011 = hash31(i + float3(0, 1, 1));
    float n111 = hash31(i + float3(1, 1, 1));

    float nx00 = lerp(n000, n100, f.x);
    float nx10 = lerp(n010, n110, f.x);
    float nx01 = lerp(n001, n101, f.x);
    float nx11 = lerp(n011, n111, f.x);
    float nxy0 = lerp(nx00, nx10, f.y);
    float nxy1 = lerp(nx01, nx11, f.y);
    return lerp(nxy0, nxy1, f.z);
}

float fbm(float3 p) {
    float result = 0.0;
    float amp = 0.55;
    [unroll] for (int i = 0; i < 5; i++) {
        result += amp * valueNoise(p);
        p = p * 2.1 + 17.3;
        amp *= 0.5;
    }
    return result;
}

void rayBox(float3 ro, float3 rd, out float tN, out float tF) {
    float3 invDir = 1.0 / rd;
    float3 t0 = (float3(-0.5, -0.5, -0.5) - ro) * invDir;
    float3 t1 = (float3(0.5, 0.5, 0.5) - ro) * invDir;
    float3 tsmaller = min(t0, t1);
    float3 tlarger  = max(t0, t1);
    tN = max(max(tsmaller.x, tsmaller.y), tsmaller.z);
    tF = min(min(tlarger.x, tlarger.y), tlarger.z);
}

float sampleDensity(float3 local, float3 worldPos) {
    float r2 = dot(local, local) * 4.0;  // local is in [-0.5, 0.5], so r2 scales to [0, 0.75] inside unit sphere
    if (r2 >= 1.0) return 0.0;
    float envelope = saturate(1.0 - r2);

    float wind = push.extras.y * push.cameraTime.w;
    float3 samplePos = worldPos * 0.45 + float3(wind, 0.0, wind * 0.3);
    float noise = fbm(samplePos);

    float3 detailPos = worldPos * 1.4 + float3(wind * 2.0, 0.0, wind * 0.6);
    float detail = fbm(detailPos);

    float coverage = push.sunCoverage.w;
    float threshold = 1.0 - coverage * envelope;
    float shape = smoothstep(threshold - 0.05, threshold + 0.05, noise);
    shape *= 1.0 - detail * 0.35;

    return saturate(shape) * push.extras.x;
}

float henyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0 - g2) / (4.0 * 3.14159265 * pow(max(denom, 1e-4), 1.5));
}

float phaseDual(float cosTheta) {
    return lerp(henyeyGreenstein(cosTheta, 0.6), henyeyGreenstein(cosTheta, -0.2), 0.3);
}

float lightMarch(float3 worldPos, float3 localPos, float3 sunDir, float3 localSunDir) {
    float tN, tF;
    rayBox(localPos, localSunDir, tN, tF);
    float travel = max(tF, 0.0);
    if (travel <= 0.0) return 1.0;

    float density = 0.0;
    float stepSize = travel / 6.0;
    float t = 0.0;
    [unroll] for (int i = 0; i < 6; i++) {
        float3 lp = localPos + localSunDir * (t + stepSize * 0.5);
        float3 wp = worldPos + sunDir * (t + stepSize * 0.5);
        density += sampleDensity(lp, wp) * stepSize;
        t += stepSize;
    }
    return exp(-density * 0.9);
}

float4 PSMain(Varyings input) : SV_Target0 {
    float3 rayDir = worldRay(input.ndc);
    float3 cameraPos = push.cameraTime.xyz;
    float3 sunDir = normalize(push.sunCoverage.xyz);

    float3 localCam = mul(push.invWorld, float4(cameraPos, 1.0)).xyz;
    float3 localDir = mul((float3x3)push.invWorld, rayDir);
    float3 localSun = normalize(mul((float3x3)push.invWorld, sunDir));

    float tN, tF;
    rayBox(localCam, localDir, tN, tF);
    tN = max(tN, 0.0);
    if (tF <= tN) return float4(0.0, 0.0, 0.0, 0.0);

    float sceneD = sceneDepth.Load(int3(input.position.xy, 0)).r;
    float sceneT = 1e9;
    if (sceneD < 1.0) {
        float3 sceneWorld = unprojectWorld(input.ndc, sceneD);
        sceneT = dot(sceneWorld - cameraPos, rayDir);
    }
    tF = min(tF, sceneT);
    if (tF <= tN) return float4(0.0, 0.0, 0.0, 0.0);

    const int STEPS = 64;
    float stepSize = (tF - tN) / float(STEPS);
    float cosTheta = dot(rayDir, sunDir);
    float ph = phaseDual(cosTheta);

    float3 sunColor = float3(1.0, 0.95, 0.85) * 7.0;
    float3 ambient = float3(0.55, 0.65, 0.80) * 0.5;

    float transmittance = 1.0;
    float3 scattered = float3(0.0, 0.0, 0.0);

    float jitter = hash31(float3(input.position.xy, push.cameraTime.w * 60.0));

    [loop] for (int i = 0; i < STEPS; i++) {
        if (transmittance < 0.01) break;
        float t = tN + stepSize * (float(i) + jitter);
        if (t > tF) break;
        float3 worldPos = cameraPos + rayDir * t;
        float3 localPos = localCam + localDir * t;
        float density = sampleDensity(localPos, worldPos);
        if (density > 0.001) {
            float lightTrans = lightMarch(worldPos, localPos, sunDir, localSun);
            float3 inScatter = (sunColor * lightTrans * ph + ambient) * density * stepSize;
            scattered += transmittance * inScatter;
            transmittance *= exp(-density * stepSize * 1.1);
        }
    }

    float alpha = 1.0 - transmittance;
    return float4(scattered, alpha);
}
