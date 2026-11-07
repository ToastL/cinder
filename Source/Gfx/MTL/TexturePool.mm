#include "Gfx/MTL/TexturePool.hpp"

#include "Gfx/MTL/Bindings.hpp"
#include "Gfx/MTL/Ctx.hpp"
#include "Gfx/MTL/MetalUtil.hpp"
#include "Gfx/MTL/Texture.hpp"

#include <stdexcept>

namespace cinder::Gfx::RHI {

TexturePool::TexturePool(const Ctx& ctx, std::uint32_t maxSets, SamplerFilter filter)
    : maxSets_(maxSets) {
    id<MTLDevice> device = cinder::Gfx::MTL::bridge<id<MTLDevice>>(ctx.device());
    MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];
    const MTLSamplerMinMagFilter metalFilter = filter == SamplerFilter::Nearest
            ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
    desc.minFilter = metalFilter;
    desc.magFilter = metalFilter;
    desc.mipFilter = MTLSamplerMipFilterNotMipmapped;
    desc.sAddressMode = MTLSamplerAddressModeClampToEdge;
    desc.tAddressMode = MTLSamplerAddressModeClampToEdge;
    id<MTLSamplerState> sampler = [device newSamplerStateWithDescriptor:desc];
    if (sampler == nil) throw cinder::Gfx::MTL::error("Could not create Metal sampler");
    sampler_ = cinder::Gfx::MTL::retain(sampler);
    bindings_.reserve(maxSets);
}

TextureBinding TexturePool::bind(void* texture) {
    if (bindings_.size() >= maxSets_) throw std::runtime_error("Metal texture pool is full");
    auto value = std::make_unique<cinder::Gfx::MTL::TextureBindingData>();
    value->texture = texture;
    value->sampler = sampler_;
    TextureBinding result = cinder::Gfx::RHI::binding(value.get());
    bindings_.push_back(std::move(value));
    return result;
}

TextureBinding TexturePool::bind(const Texture& texture) { return bind(texture.handle()); }

TexturePool::~TexturePool() {
    bindings_.clear();
    cinder::Gfx::MTL::release(sampler_);
}

}
