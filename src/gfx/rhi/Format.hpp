#pragma once

#include <cstdint>

namespace cinder::gfx::rhi {

enum class Format : std::uint8_t {
    Undefined,
    BGRA8Srgb,
    BGRA8Unorm,
    RGBA8Srgb,
    ABGR8SrgbPack32,
    R8Unorm,
    D32Sfloat,
    D32SfloatS8Uint,
    D24UnormS8Uint,
};

enum class VertexFormat : std::uint8_t {
    Float2,
    Float3,
    Float4,
};

enum class PassKind : std::uint8_t {
    Scene,
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
