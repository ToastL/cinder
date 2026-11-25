#pragma once

#include <cstdint>

namespace cinder::Gfx::RHI {

enum class Format : std::uint8_t {
    Undefined,
    BGRA8Srgb,
    BGRA8Unorm,
    RGBA8Srgb,
    RGBA8Unorm,
    ABGR8SrgbPack32,
    R8Unorm,
    RGBA16Sfloat,
    D32Sfloat,
    D32SfloatS8Uint,
    D24UnormS8Uint,
};

enum class VertexFormat : std::uint8_t {
    Float2,
    Float3,
    Float4,
};

constexpr std::uint32_t MAX_TEXTURE_SLOTS = 4;

enum class PassKind : std::uint8_t {
    Scene,
    Overlay,
    Resolve,
    Present,
};

enum class ShaderStages : std::uint8_t {
    Vertex,
    Fragment,
    Both,
};

enum class BufferUsage : std::uint8_t {
    Vertex,
    Index,
    TransferSrc,
    TransferDst,
};

enum class SamplerFilter : std::uint8_t {
    Nearest,
    Linear,
};

enum class SamplerAddress : std::uint8_t {
    ClampToEdge,
    Repeat,
};

enum class TextureType : std::uint8_t {
    Tex2D,
    Tex3D,
};

struct SamplerDesc {
    SamplerFilter filter = SamplerFilter::Linear;
    SamplerAddress address = SamplerAddress::ClampToEdge;
};

enum class Winding : std::uint8_t {
    Clockwise,
    CounterClockwise,
};

constexpr bool isSrgb(Format format) {
    return format == Format::BGRA8Srgb || format == Format::RGBA8Srgb
            || format == Format::ABGR8SrgbPack32;
}

constexpr bool isBgra(Format format) {
    return format == Format::BGRA8Srgb || format == Format::BGRA8Unorm;
}

constexpr bool isDepth(Format format) {
    return format == Format::D32Sfloat || format == Format::D32SfloatS8Uint
            || format == Format::D24UnormS8Uint;
}

}
