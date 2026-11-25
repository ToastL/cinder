struct Push {
    float4x4 invViewProj;
    float4 cameraTime;
    float4 sunCoverage;
    float4 layer;
    float4 scales;
};

[[vk::push_constant]] ConstantBuffer<Push> push : register(b0);

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D sceneDepth : register(t0);
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState depthSampler : register(s0);
[[vk::combinedImageSampler]] [[vk::binding(1, 1)]] Texture3D shapeNoise : register(t1);
[[vk::combinedImageSampler]] [[vk::binding(1, 1)]] SamplerState shapeSampler : register(s1);
[[vk::combinedImageSampler]] [[vk::binding(2, 2)]] Texture3D detailNoise : register(t2);
[[vk::combinedImageSampler]] [[vk::binding(2, 2)]] SamplerState detailSampler : register(s2);
[[vk::combinedImageSampler]] [[vk::binding(3, 3)]] Texture3D weatherMap : register(t3);
[[vk::combinedImageSampler]] [[vk::binding(3, 3)]] SamplerState weatherSampler : register(s3);

struct Varyings {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 ndc : NDC;
};

static const int MAX_STEPS = 128;
static const float COARSE_STEPS = 48.0;
static const float FINE_STEP = 0.35;
static const int EMPTY_FINE_STEPS = 6;
static const int LIGHT_STEPS = 6;
static const float EXTINCTION = 1.0;
static const float3 SUN_COLOR = float3(1.0, 0.95, 0.85) * 6.0;
static const float3 AMBIENT_LOW = float3(0.45, 0.50, 0.60);
static const float3 AMBIENT_HIGH = float3(0.80, 0.86, 0.95);
static const int SCATTER_OCTAVES = 3;
static const float DETAIL_EROSION = 0.4;

Varyings VSMain(uint vertex : SV_VertexID) {
    Varyings output;
    float2 uv = float2((vertex << 1) & 2, vertex & 2);
    float2 clip = uv * 2.0 - 1.0;
    output.position = float4(clip, 1.0, 1.0);
    output.ndc = clip;
    return output;
}

float remap(float value, float low, float high, float newLow, float newHigh) {
    return newLow + (value - low) * (newHigh - newLow) / (high - low);
}

float3 unprojectWorld(float2 ndc, float depth) {
    float4 h = mul(push.invViewProj, float4(ndc, depth, 1.0));
    return h.xyz / h.w;
}

float3 worldRay(float2 ndc) {
    return normalize(unprojectWorld(ndc, 1.0) - unprojectWorld(ndc, 0.0));
}

bool layerSpan(float3 origin, float3 dir, out float tNear, out float tFar) {
    float bottom = push.layer.x;
    float top = push.layer.y;
    if (abs(dir.y) < 1e-5) {
        tNear = 0.0;
        tFar = push.layer.z;
        return origin.y > bottom && origin.y < top;
    }
    float t0 = (bottom - origin.y) / dir.y;
    float t1 = (top - origin.y) / dir.y;
    tNear = max(min(t0, t1), 0.0);
    tFar = min(max(t0, t1), push.layer.z);
    return tFar > tNear;
}

float heightFraction(float y) {
    return saturate((y - push.layer.x) / (push.layer.y - push.layer.x));
}

float heightGradient(float h, float type, float baseOffset) {
    float base = baseOffset * 0.35;
    float height = lerp(0.3, 1.0 - base, type);
    float rise = saturate(remap(h, base, base + height * 0.1, 0.0, 1.0));
    float fall = saturate(remap(h, base + height * 0.35, base + height, 1.0, 0.0));
    return rise * smoothstep(0.0, 1.0, fall);
}

float3 wind() {
    return float3(1.0, 0.0, 0.3) * push.cameraTime.w;
}

float density(float3 p, bool detailed) {
    float h = heightFraction(p.y);
    if (h <= 0.0 || h >= 1.0) return 0.0;

    float coverage = push.sunCoverage.w;
    if (coverage <= 0.0) return 0.0;
    float3 moved = p + wind();
    float4 weather = weatherMap.SampleLevel(weatherSampler, float3(moved.xz * push.scales.x, 0.5), 0.0);
    float local = saturate(remap(weather.r, 1.0 - coverage, 1.0, 0.0, 1.0));
    if (local <= 0.0) return 0.0;

    float4 shape = shapeNoise.SampleLevel(shapeSampler, moved * push.scales.y, 0.0);
    float worleyFbm = shape.g * 0.625 + shape.b * 0.25 + shape.a * 0.125;
    float base = remap(shape.r, worleyFbm - 1.0, 1.0, 0.0, 1.0) * heightGradient(h, weather.g, weather.b);
    float cloud = saturate(remap(base, 1.0 - local, 1.0, 0.0, 1.0)) * local;
    if (cloud <= 0.0) return 0.0;

    if (detailed) {
        float3 detail = detailNoise.SampleLevel(detailSampler, (p + wind() * 2.0) * push.scales.z, 0.0).rgb;
        float detailFbm = detail.r * 0.625 + detail.g * 0.25 + detail.b * 0.125;
        float erosion = lerp(detailFbm, 1.0 - detailFbm, saturate(h * 8.0)) * DETAIL_EROSION;
        cloud = saturate(remap(cloud, erosion, 1.0, 0.0, 1.0));
    }
    return cloud * push.layer.w;
}

float henyeyGreenstein(float cosTheta, float g) {
    float g2 = g * g;
    float denom = 1.0 + g2 - 2.0 * g * cosTheta;
    return (1.0 - g2) / (4.0 * 3.14159265 * pow(max(denom, 1e-4), 1.5));
}

float phase(float cosTheta, float eccentricity) {
    return lerp(henyeyGreenstein(cosTheta, 0.6 * eccentricity), henyeyGreenstein(cosTheta, -0.2 * eccentricity), 0.3);
}

float sunlight(float3 p, float3 sunDir, float3 phases) {
    float stepLength = (push.layer.y - push.layer.x) * 0.06;
    float opticalDepth = 0.0;
    float travelled = 0.0;
    [unroll] for (int i = 0; i < LIGHT_STEPS; i++) {
        float segment = stepLength * float(i + 1);
        travelled += segment;
        opticalDepth += density(p + sunDir * (travelled - segment * 0.5), false) * segment;
    }
    float far = (push.layer.y - push.layer.x) * 1.5;
    opticalDepth += density(p + sunDir * (travelled + far * 0.5), false) * far;

    float energy = 0.0;
    float weight = 1.0;
    float extinction = EXTINCTION;
    [unroll] for (int octave = 0; octave < SCATTER_OCTAVES; octave++) {
        energy += weight * exp(-opticalDepth * extinction) * phases[octave];
        weight *= 0.5;
        extinction *= 0.5;
    }
    float powder = 1.0 - exp(-opticalDepth * EXTINCTION * 2.0);
    return energy * lerp(1.0, powder, 0.25);
}

int resolutionScale() {
    return int(push.scales.w) / 4;
}

int2 frameOffset() {
    int index = int(push.scales.w) % 4;
    return int2(index == 1 || index == 2 ? 1 : 0, index == 1 || index == 3 ? 1 : 0);
}

float closestDepth(int2 lowPixel, float2 fullSize) {
    int scale = resolutionScale();
    float closest = 1.0;
    for (int y = 0; y < scale; y++) {
        for (int x = 0; x < scale; x++) {
            float2 uv = min((float2(lowPixel * scale + int2(x, y)) + 0.5) / fullSize, 1.0);
            closest = min(closest, sceneDepth.SampleLevel(depthSampler, uv, 0.0).r);
        }
    }
    return closest;
}

float interleavedGradientNoise(float2 pixel) {
    return frac(52.9829189 * frac(dot(pixel, float2(0.06711056, 0.00583715))));
}

float4 PSMain(Varyings input) : SV_Target0 {
    uint fullWidth, fullHeight;
    sceneDepth.GetDimensions(fullWidth, fullHeight);
    float2 fullSize = float2(fullWidth, fullHeight);
    int scale = resolutionScale();
    int2 lowSize = (int2(fullWidth, fullHeight) + scale - 1) / scale;
    int2 freshSize = (lowSize + 1) / 2;

    int2 fresh = int2(floor((input.ndc * 0.5 + 0.5) * float2(freshSize)));
    int2 lowPixel = min(fresh * 2 + frameOffset(), lowSize - 1);
    float2 ndc = (float2(lowPixel) + 0.5) / float2(lowSize) * 2.0 - 1.0;

    float3 rayDir = worldRay(ndc);
    float3 cameraPos = push.cameraTime.xyz;
    float3 sunDir = normalize(push.sunCoverage.xyz);

    float tNear, tFar;
    if (!layerSpan(cameraPos, rayDir, tNear, tFar)) return float4(0.0, 0.0, 0.0, 0.0);

    float sceneD = closestDepth(lowPixel, fullSize);
    if (sceneD < 1.0) {
        float3 sceneWorld = unprojectWorld(ndc, sceneD);
        tFar = min(tFar, dot(sceneWorld - cameraPos, rayDir));
    }
    if (tFar <= tNear) return float4(0.0, 0.0, 0.0, 0.0);

    float coarse = (tFar - tNear) / COARSE_STEPS;
    float fine = coarse * FINE_STEP;
    float cosTheta = dot(rayDir, sunDir);
    float3 phases = float3(phase(cosTheta, 1.0), phase(cosTheta, 0.5), phase(cosTheta, 0.25));

    float t = tNear + coarse * interleavedGradientNoise(float2(lowPixel));
    float transmittance = 1.0;
    float3 scattered = float3(0.0, 0.0, 0.0);
    bool inCloud = false;
    int empty = 0;

    [loop] for (int i = 0; i < MAX_STEPS; i++) {
        if (t >= tFar || transmittance < 0.01) break;
        float3 p = cameraPos + rayDir * t;

        if (!inCloud) {
            if (density(p, false) > 0.0) {
                inCloud = true;
                empty = 0;
                t = max(t - coarse, tNear);
                continue;
            }
            t += coarse;
            continue;
        }

        float d = density(p, true);
        if (d > 0.0) {
            empty = 0;
            float3 ambient = lerp(AMBIENT_LOW, AMBIENT_HIGH, heightFraction(p.y));
            float3 light = SUN_COLOR * sunlight(p, sunDir, phases) + ambient;
            float absorbed = 1.0 - exp(-d * fine * EXTINCTION);
            scattered += transmittance * absorbed * light;
            transmittance *= 1.0 - absorbed;
        } else if (++empty >= EMPTY_FINE_STEPS) {
            inCloud = false;
        }
        t += fine;
    }

    float fade = saturate(1.0 - tNear / push.layer.z);
    return float4(scattered * fade, (1.0 - transmittance) * fade);
}
