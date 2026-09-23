struct Push {
    float2 viewport;
    float encode;
    float textGamma;
    float pixelsPerPoint;
};

[[vk::push_constant]] Push push;

[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] Texture2D image;
[[vk::combinedImageSampler]] [[vk::binding(0, 0)]] SamplerState imageSampler;

struct Vertex {
    [[vk::location(0)]] float2 position : POSITION;
    [[vk::location(1)]] float2 uv : TEXCOORD0;
    [[vk::location(2)]] float4 color : COLOR0;
    [[vk::location(3)]] float4 border : COLOR1;
    [[vk::location(4)]] float4 shape : TEXCOORD1;
    [[vk::location(5)]] float4 radii : TEXCOORD2;
};

struct Varyings {
    float4 position : SV_Position;
    [[vk::location(0)]] float2 uv : TEXCOORD0;
    [[vk::location(1)]] float4 color : COLOR0;
    [[vk::location(2)]] float4 border : COLOR1;
    [[vk::location(3)]] nointerpolation float4 shape : TEXCOORD1;
    [[vk::location(4)]] nointerpolation float4 radii : TEXCOORD2;
};

static const int SOLID = 0;
static const int TEXTURED = 1;
static const int GLYPH = 2;
static const int BOX = 3;
static const int OPAQUE = 4;

float roundedBox(float2 p, float2 extent, float4 radii) {
    float radius = p.x < 0.0 ? (p.y < 0.0 ? radii.x : radii.w) : (p.y < 0.0 ? radii.y : radii.z);
    float2 corner = abs(p) - extent + radius;
    return min(max(corner.x, corner.y), 0.0) + length(max(corner, 0.0)) - radius;
}

float3 encodeSrgb(float3 value) {
    float3 low = value * 12.92;
    float3 high = 1.055 * pow(value, 1.0 / 2.4) - 0.055;
    return lerp(low, high, step(0.0031308, value));
}

float4 premultiply(float4 color) {
    return float4(color.rgb * color.a, color.a);
}

Varyings VSMain(Vertex input) {
    Varyings output;
    output.position = float4(input.position / push.viewport * 2.0 - 1.0, 0.0, 1.0);
    output.uv = input.uv;
    output.color = input.color;
    output.border = input.border;
    output.shape = input.shape;
    output.radii = input.radii;
    return output;
}

float4 PSMain(Varyings input) : SV_Target0 {
    float4 sampled = image.Sample(imageSampler, input.uv);
    int mode = int(input.shape.w + 0.5);
    float4 color;

    if (mode == TEXTURED) {
        color = premultiply(sampled * input.color);
    } else if (mode == GLYPH) {
        float coverage = pow(sampled.r, push.textGamma);
        color = premultiply(float4(input.color.rgb, input.color.a * coverage));
    } else if (mode == BOX) {
        float distance = roundedBox(input.uv, input.shape.xy, input.radii) * push.pixelsPerPoint;
        float border = input.shape.z * push.pixelsPerPoint;
        float outer = clamp(0.5 - distance, 0.0, 1.0);
        float inner = border > 0.0 ? clamp(0.5 - distance - border, 0.0, 1.0) : outer;
        color = premultiply(input.color) * inner + premultiply(input.border) * (outer - inner);
    } else if (mode == OPAQUE) {
        color = premultiply(float4(sampled.rgb * input.color.rgb, input.color.a));
    } else {
        color = premultiply(input.color);
    }

    if (push.encode > 0.5 && color.a > 0.0) {
        color.rgb = encodeSrgb(color.rgb / color.a) * color.a;
    }
    return color;
}
