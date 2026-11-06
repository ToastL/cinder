#include "gfx/rhi/Commands.hpp"

#include "gfx/mtl/Commands.hpp"
#include "gfx/mtl/MetalUtil.hpp"

namespace cinder::gfx::rhi {

void draw(Commands cmd, std::uint32_t vertexCount) {
    auto& state = unwrap(cmd);
    id<MTLRenderCommandEncoder> encoder =
            cinder::gfx::mtl::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    [encoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:vertexCount];
}

void drawIndexed(Commands cmd, std::uint32_t indexCount, std::uint32_t firstIndex) {
    auto& state = unwrap(cmd);
    id<MTLRenderCommandEncoder> encoder =
            cinder::gfx::mtl::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    id<MTLBuffer> indices = cinder::gfx::mtl::bridge<id<MTLBuffer>>(state.indexBuffer);
    [encoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:indexCount
                         indexType:MTLIndexTypeUInt32 indexBuffer:indices
                 indexBufferOffset:static_cast<NSUInteger>(firstIndex) * sizeof(std::uint32_t)];
}

void viewport(Commands cmd, std::uint32_t width, std::uint32_t height) {
    auto& state = unwrap(cmd);
    id<MTLRenderCommandEncoder> encoder =
            cinder::gfx::mtl::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    [encoder setViewport:MTLViewport{0.0, 0.0, static_cast<double>(width),
                                     static_cast<double>(height), 0.0, 1.0}];
}

void scissor(Commands cmd, std::int32_t x, std::int32_t y, std::uint32_t width,
             std::uint32_t height) {
    auto& state = unwrap(cmd);
    id<MTLRenderCommandEncoder> encoder =
            cinder::gfx::mtl::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    const NSUInteger left = static_cast<NSUInteger>(x < 0 ? 0 : x);
    const NSUInteger top = static_cast<NSUInteger>(y < 0 ? 0 : y);
    [encoder setScissorRect:MTLScissorRect{left, top, width, height}];
}

}
