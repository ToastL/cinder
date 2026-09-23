#include <doctest/doctest.h>

#include "gfx/rhi/Format.hpp"
#include "gfx/vk/Formats.hpp"

using cinder::gfx::rhi::Format;
using cinder::gfx::rhi::fromVk;
using cinder::gfx::rhi::isBgra;
using cinder::gfx::rhi::isDepth;
using cinder::gfx::rhi::isSrgb;
using cinder::gfx::rhi::toVk;
using cinder::gfx::rhi::VertexFormat;

namespace {

constexpr Format ALL[] = {
    Format::Undefined,       Format::BGRA8Srgb,       Format::BGRA8Unorm,
    Format::RGBA8Srgb,       Format::ABGR8SrgbPack32, Format::R8Unorm,
    Format::D32Sfloat,       Format::D32SfloatS8Uint, Format::D24UnormS8Uint,
};

}

TEST_CASE("every format round-trips through Vulkan") {
    for (Format format : ALL) {
        CHECK(fromVk(toVk(format)) == format);
    }
}

TEST_CASE("only real sRGB formats report sRGB") {
    CHECK(isSrgb(Format::BGRA8Srgb));
    CHECK(isSrgb(Format::RGBA8Srgb));
    CHECK(isSrgb(Format::ABGR8SrgbPack32));

    CHECK_FALSE(isSrgb(Format::BGRA8Unorm));
    CHECK_FALSE(isSrgb(Format::R8Unorm));
    CHECK_FALSE(isSrgb(Format::Undefined));
}

TEST_CASE("only blue-first formats report BGRA") {
    CHECK(isBgra(Format::BGRA8Srgb));
    CHECK(isBgra(Format::BGRA8Unorm));

    CHECK_FALSE(isBgra(Format::RGBA8Srgb));
    CHECK_FALSE(isBgra(Format::ABGR8SrgbPack32));
    CHECK_FALSE(isBgra(Format::Undefined));
}

TEST_CASE("depth formats are exactly the three the device may choose") {
    CHECK(isDepth(Format::D32Sfloat));
    CHECK(isDepth(Format::D32SfloatS8Uint));
    CHECK(isDepth(Format::D24UnormS8Uint));

    for (Format format : ALL) {
        const bool bothColourAndDepth = isDepth(format) && isSrgb(format);
        CHECK_FALSE(bothColourAndDepth);
    }
}

TEST_CASE("the sRGB and BGRA tables match the Vulkan constants they replaced") {
    CHECK(isSrgb(fromVk(VK_FORMAT_B8G8R8A8_SRGB)));
    CHECK(isSrgb(fromVk(VK_FORMAT_R8G8B8A8_SRGB)));
    CHECK(isSrgb(fromVk(VK_FORMAT_A8B8G8R8_SRGB_PACK32)));
    CHECK_FALSE(isSrgb(fromVk(VK_FORMAT_B8G8R8A8_UNORM)));

    CHECK(isBgra(fromVk(VK_FORMAT_B8G8R8A8_SRGB)));
    CHECK(isBgra(fromVk(VK_FORMAT_B8G8R8A8_UNORM)));
    CHECK_FALSE(isBgra(fromVk(VK_FORMAT_R8G8B8A8_SRGB)));
}

TEST_CASE("vertex formats map to the sizes the pipelines declare") {
    CHECK(toVk(VertexFormat::Float2) == VK_FORMAT_R32G32_SFLOAT);
    CHECK(toVk(VertexFormat::Float3) == VK_FORMAT_R32G32B32_SFLOAT);
    CHECK(toVk(VertexFormat::Float4) == VK_FORMAT_R32G32B32A32_SFLOAT);
}
