#include "Gfx/MTL/GlyphPages.hpp"

#include "Gfx/MTL/Commands.hpp"
#include "Gfx/MTL/Ctx.hpp"
#include "Gfx/MTL/GpuBuffer.hpp"
#include "Gfx/MTL/MetalUtil.hpp"
#include "Gfx/MTL/TexturePool.hpp"

namespace cinder::Gfx::RHI {

GlyphPages::GlyphPages(const Ctx& ctx, std::uint32_t size) : ctx_(ctx), size_(size) {}

GlyphPages::Page& GlyphPages::page(std::size_t index) {
    while (pages_.size() <= index) {
        Page& created = pages_.emplace_back();
        id<MTLDevice> device = cinder::Gfx::MTL::bridge<id<MTLDevice>>(ctx_.device());
        MTLTextureDescriptor* desc = [MTLTextureDescriptor
                texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                             width:size_ height:size_ mipmapped:NO];
        desc.storageMode = MTLStorageModeShared;
        desc.usage = MTLTextureUsageShaderRead;
        id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
        if (texture == nil) throw cinder::Gfx::MTL::error("Could not create Metal glyph page");
        created.texture = cinder::Gfx::MTL::retain(texture);
        created.pool = std::make_unique<TexturePool>(ctx_, 1, SamplerFilter::Linear);
        created.binding = created.pool->bind(created.texture);
    }
    return pages_[index];
}

TextureBinding GlyphPages::binding(std::size_t index) { return page(index).binding; }
bool GlyphPages::uploaded(std::size_t index) { return page(index).uploaded; }

void GlyphPages::forgetUploads() {
    for (Page& value : pages_) value.uploaded = false;
}

void GlyphPages::upload(Uploads cmd, const GpuBuffer& staging, const std::vector<GlyphRegion>& regions) {
    if (regions.empty()) return;
    id<MTLCommandBuffer> buffer = cinder::Gfx::MTL::bridge<id<MTLCommandBuffer>>(cinder::Gfx::RHI::unwrap(cmd));
    id<MTLBuffer> source = cinder::Gfx::MTL::bridge<id<MTLBuffer>>(staging.handle());
    id<MTLBlitCommandEncoder> blit = [buffer blitCommandEncoder];
    if (blit == nil) throw cinder::Gfx::MTL::error("Could not begin Metal glyph upload");
    for (const GlyphRegion& region : regions) {
        Page& target = page(region.page);
        target.uploaded = true;
        if (region.width <= 0 || region.height <= 0) continue;
        const auto width = static_cast<NSUInteger>(region.width);
        const auto height = static_cast<NSUInteger>(region.height);
        id<MTLTexture> texture = cinder::Gfx::MTL::bridge<id<MTLTexture>>(target.texture);
        [blit copyFromBuffer:source
                   sourceOffset:static_cast<NSUInteger>(region.offset)
              sourceBytesPerRow:width
            sourceBytesPerImage:width * height
                     sourceSize:MTLSizeMake(width, height, 1)
                      toTexture:texture
               destinationSlice:0
               destinationLevel:0
              destinationOrigin:MTLOriginMake(static_cast<NSUInteger>(region.x), static_cast<NSUInteger>(region.y), 0)];
    }
    [blit endEncoding];
}

GlyphPages::~GlyphPages() {
    for (Page& value : pages_) {
        value.pool.reset();
        cinder::Gfx::MTL::release(value.texture);
    }
}

}
