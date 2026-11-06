#include "gfx/mtl/Formats.hpp"

#import <Metal/Metal.h>

namespace cinder::gfx::mtl {

std::uint64_t pixelFormat(cinder::gfx::rhi::Format format) {
    using cinder::gfx::rhi::Format;
    switch (format) {
        case Format::Undefined: return MTLPixelFormatInvalid;
        case Format::BGRA8Srgb: return MTLPixelFormatBGRA8Unorm_sRGB;
        case Format::BGRA8Unorm: return MTLPixelFormatBGRA8Unorm;
        case Format::RGBA8Srgb: return MTLPixelFormatRGBA8Unorm_sRGB;
        case Format::ABGR8SrgbPack32: return MTLPixelFormatRGBA8Unorm_sRGB;
        case Format::R8Unorm: return MTLPixelFormatR8Unorm;
        case Format::D32Sfloat: return MTLPixelFormatDepth32Float;
        case Format::D32SfloatS8Uint: return MTLPixelFormatDepth32Float_Stencil8;
        case Format::D24UnormS8Uint: return MTLPixelFormatDepth24Unorm_Stencil8;
    }
    return MTLPixelFormatInvalid;
}

std::uint64_t vertexFormat(cinder::gfx::rhi::VertexFormat format) {
    using cinder::gfx::rhi::VertexFormat;
    switch (format) {
        case VertexFormat::Float2: return MTLVertexFormatFloat2;
        case VertexFormat::Float3: return MTLVertexFormatFloat3;
        case VertexFormat::Float4: return MTLVertexFormatFloat4;
    }
    return MTLVertexFormatInvalid;
}

}
