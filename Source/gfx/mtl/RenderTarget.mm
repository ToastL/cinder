#include "gfx/mtl/RenderTarget.hpp"

#include "gfx/mtl/Ctx.hpp"
#include "gfx/mtl/Formats.hpp"
#include "gfx/mtl/MetalUtil.hpp"
#include "gfx/mtl/TexturePool.hpp"

namespace cinder::gfx::rhi {

RenderTarget::RenderTarget(const Ctx& ctx, Format format, std::uint32_t width,
                           std::uint32_t height)
    : width_(width), height_(height) {
    id<MTLDevice> device = cinder::gfx::mtl::bridge<id<MTLDevice>>(ctx.device());

    MTLTextureDescriptor* colorDesc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:static_cast<MTLPixelFormat>(
                                                       cinder::gfx::mtl::pixelFormat(format))
                                         width:width height:height mipmapped:NO];
    colorDesc.storageMode = MTLStorageModePrivate;
    colorDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    id<MTLTexture> color = [device newTextureWithDescriptor:colorDesc];
    if (color == nil) throw cinder::gfx::mtl::error("Could not create Metal render target");

    MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
                                         width:width height:height mipmapped:NO];
    depthDesc.storageMode = MTLStorageModePrivate;
    depthDesc.usage = MTLTextureUsageRenderTarget;
    id<MTLTexture> depth = [device newTextureWithDescriptor:depthDesc];
    if (depth == nil) throw cinder::gfx::mtl::error("Could not create Metal depth target");

    color_ = cinder::gfx::mtl::retain(color);
    depth_ = cinder::gfx::mtl::retain(depth);
    pool_ = std::make_unique<TexturePool>(ctx, 1, SamplerFilter::Linear);
    binding_ = pool_->bind(color_);
}

RenderTarget::~RenderTarget() {
    pool_.reset();
    cinder::gfx::mtl::release(depth_);
    cinder::gfx::mtl::release(color_);
}

}
