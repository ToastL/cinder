#include "gfx/mtl/TexturePool.hpp"

#include "gfx/mtl/Bindings.hpp"
#include "gfx/mtl/Ctx.hpp"
#include "gfx/mtl/MetalUtil.hpp"
#include "gfx/mtl/Texture.hpp"

#include <stdexcept>

namespace cinder::gfx::rhi {

TexturePool::TexturePool(const Ctx& ctx, std::uint32_t maxSets, SamplerFilter filter)
    : maxSets_(maxSets) {
    id<MTLDevice> device = cinder::gfx::mtl::bridge<id<MTLDevice>>(ctx.device());
    MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];
    const MTLSamplerMinMagFilter metalFilter = filter == SamplerFilter::Nearest
            ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
    desc.minFilter = metalFilter;
    desc.magFilter = metalFilter;
    desc.mipFilter = MTLSamplerMipFilterNotMipmapped;
    desc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    desc.tAddressMode = MTLSamplerAddressModeClampToEdge;
    id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor:desc];
    if (sampler == nil) throw cinder::gfx::mtl::error("Could not create Metal sampler");
    sampler_ = cinder::gfx::mtl::retain(sampler);
    bindings_.reserve(maxSets);
}

TextureBinding TexturePool::bind(void* texture) {
    if (bindings_.size() >= maxSets_) throw std::runtime_error("Metal texture pool is full");
    auto value = std::make_unique<cinder::gfx::mtl::TextureBindingData>();
    value->texture = texture;
    value->sampler = sampler_;
    TextureBinding result = cinder::gfx::rhi::binding(value.get());
    bindings_.push_back(std::move(value));
    return result;
}

TextureBinding TexturePool::bind(const Texture& texture) { return bind(texture.handle()); }

TexturePool::~TexturePool() {
    bindings_.clear();
    cinder::gfx::mtl::release(sampler_);
}

}
