#include "Gfx/MTL/RenderTarget.hpp"

#include "Gfx/MTL/Ctx.hpp"
#include "Gfx/MTL/Formats.hpp"
#include "Gfx/MTL/MetalUtil.hpp"
#include "Gfx/MTL/TexturePool.hpp"

namespace cinder::Gfx::RHI {

namespace {

id<MTLTexture> colorTexture(id<MTLDevice> device, Format format, std::uint32_t width,
                            std::uint32_t height) {
    MTLTextureDescriptor* desc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:static_cast<MTLPixelFormat>(
                                                       cinder::Gfx::MTL::pixelFormat(format))
                                         width:width height:height mipmapped:NO];
    desc.storageMode = MTLStorageModePrivate;
    desc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    id<MTLTexture> texture = [device newTextureWithDescriptor:desc];
    if (texture == nil) throw cinder::Gfx::MTL::error("Could not create Metal render target");
    return texture;
}

}

RenderTarget::RenderTarget(const Ctx& ctx, Format sceneFormat, Format resolvedFormat,
                           std::uint32_t width, std::uint32_t height)
    : width_(width), height_(height) {
    const cinder::Gfx::MTL::PoolScope pool;
    id<MTLDevice> device = cinder::Gfx::MTL::bridge<id<MTLDevice>>(ctx.device());

    id<MTLTexture> color = colorTexture(device, sceneFormat, width, height);
    id<MTLTexture> resolved = colorTexture(device, resolvedFormat, width, height);

    MTLTextureDescriptor* depthDesc = [MTLTextureDescriptor
            texture2DDescriptorWithPixelFormat:static_cast<MTLPixelFormat>(
                                                       cinder::Gfx::MTL::pixelFormat(ctx.depthFormat()))
                                         width:width height:height mipmapped:NO];
    depthDesc.storageMode = MTLStorageModePrivate;
    depthDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    id<MTLTexture> depth = [device newTextureWithDescriptor:depthDesc];
    if (depth == nil) throw cinder::Gfx::MTL::error("Could not create Metal depth target");

    color_ = cinder::Gfx::MTL::retain(color);
    depth_ = cinder::Gfx::MTL::retain(depth);
    resolved_ = cinder::Gfx::MTL::retain(resolved);
    pool_ = std::make_unique<TexturePool>(ctx, 1, SamplerFilter::Linear);
    binding_ = pool_->bind(color_);
    depthPool_ = std::make_unique<TexturePool>(ctx, 1, SamplerFilter::Nearest);
    depthBinding_ = depthPool_->bind(depth_);
    resolvedPool_ = std::make_unique<TexturePool>(ctx, 1, SamplerFilter::Linear);
    resolvedBinding_ = resolvedPool_->bind(resolved_);
}

RenderTarget::~RenderTarget() {
    resolvedPool_.reset();
    depthPool_.reset();
    pool_.reset();
    cinder::Gfx::MTL::release(resolved_);
    cinder::Gfx::MTL::release(depth_);
    cinder::Gfx::MTL::release(color_);
}

}
