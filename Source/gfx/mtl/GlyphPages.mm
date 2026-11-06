#include "gfx/mtl/GlyphPages.hpp"

#include "gfx/mtl/Ctx.hpp"
#include "gfx/mtl/GpuBuffer.hpp"
#include "gfx/mtl/MetalUtil.hpp"
#include "gfx/mtl/TexturePool.hpp"

namespace cinder::gfx::rhi {

GlyphPages::GlyphPages(const Ctx& ctx, std::uint32_t size) : ctx_(ctx), size_(size) {}

GlyphPages::Page& GlyphPages::page(std::size_t index) {
    while (pages_.size() <= index) {
        Page& created = pages_.emplace_back();
        id<MTLDevice> device = cinder::gfx::mtl::bridge<id<MTLDevice>>(ctx_.device());
        MTLTextureDescriptor* desc = [MTLTextureDescriptor
                texture2DDescriptorWithPixelFormat:MTLPixelFormatR8Unorm
                                             width:size_ height:size_ mipmapped:NO];
        desc.storageMode = MTLStorageModeShared;
        desc.usage = MTLTextureUsageShaderRead;
        id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
        if (texture == nil) throw cinder::gfx::mtl::error("Could not create Metal glyph page");
        created.texture = cinder::gfx::mtl::retain(texture);
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

void GlyphPages::upload(Uploads, const GpuBuffer& staging,
                        const std::vector<GlyphRegion>& regions) {
    const auto* source = static_cast<const std::uint8_t*>(staging.mapped());
    for (const GlyphRegion& region : regions) {
        Page& target = page(region.page);
        id<MTLTexture> texture = cinder::gfx::mtl::bridge<id<MTLTexture>>(target.texture);
        [texture replaceRegion:MTLRegionMake2D(region.x, region.y, region.width, region.height)
                    mipmapLevel:0 withBytes:source + region.offset
                    bytesPerRow:static_cast<NSUInteger>(region.width)];
        target.uploaded = true;
    }
}

GlyphPages::~GlyphPages() {
    for (Page& value : pages_) {
        value.pool.reset();
        cinder::gfx::mtl::release(value.texture);
    }
}

}
