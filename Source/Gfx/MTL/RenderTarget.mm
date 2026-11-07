#include "Gfx/MTL/RenderTarget.hpp"

#include "Gfx/MTL/Ctx.hpp"
#include "Gfx/MTL/Formats.hpp"
#include "Gfx/MTL/MetalUtil.hpp"
#include "Gfx/MTL/TexturePool.hpp"

namespace cinder::Gfx::RHI {

RenderTarget::RenderTarget(const Ctx& ctx, Format format, std::uint32_t width,
                           std::uint32_t height)
    : width_(width), height_(height) {
    const cinder::Gfx::MTL::PoolScope pool;
    id<MTLDevice> device = cinder::Gfx::MTL::bridge<id<MTLDevice>>(ctx.device());

    MTLTextureDescriptor* colorDesc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:static_cast<MTLPixelFormat>(
                                                       cinder::Gfx::MTL::pixelFormat(format))
                                         width:width height:height mipmapped:NO];
    colorDesc.storageMode = MTLStorageModePrivate;
    colorDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    id<MTLTexture> color = [device newTextureWithDescriptor:colorDesc];
    if (color == nil) throw cinder::Gfx::MTL::error("Could not create Metal render target");

    MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:static_cast<MTLPixelFormat>(
                                                       cinder::Gfx::MTL::pixelFormat(ctx.depthFormat()))
                                         width:width height:height mipmapped:NO];
    depthDesc.storageMode = MTLStorageModePrivate;
    depthDesc.usage = MTLTextureUsageRenderTarget;
    id<MTLTexture> depth = [device newTextureWithDescriptor:depthDesc];
    if (depth == nil) throw cinder::Gfx::MTL::error("Could not create Metal depth target");

    color_ = cinder::Gfx::MTL::retain(color);
    depth_ = cinder::Gfx::MTL::retain(depth);
    pool_ = std::make_unique<TexturePool>(ctx, 1, SamplerFilter::Linear);
    binding_ = pool_->bind(color_);
}

RenderTarget::~RenderTarget() {
    pool_.reset();
    cinder::Gfx::MTL::release(depth_);
    cinder::Gfx::MTL::release(color_);
}

}
