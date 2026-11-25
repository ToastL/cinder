#include "Gfx/MTL/TexturePool.hpp"

#include "Gfx/MTL/Bindings.hpp"
#include "Gfx/MTL/Ctx.hpp"
#include "Gfx/MTL/MetalUtil.hpp"
#include "Gfx/MTL/Texture.hpp"
#include "Gfx/MTL/VolumeTexture.hpp"

#include <stdexcept>

namespace cinder::Gfx::RHI {

TexturePool::TexturePool(const Ctx& ctx, std::uint32_t maxSets, SamplerFilter filter)
    : TexturePool(ctx, maxSets, SamplerDesc{filter, SamplerAddress::ClampToEdge}) {}

TexturePool::TexturePool(const Ctx& ctx, std::uint32_t maxSets, SamplerDesc samplerDesc)
    : maxSets_(maxSets) {
    id<MTLDevice> device = cinder::Gfx::MTL::bridge<id<MTLDevice>>(ctx.device());
    MTLSamplerDescriptor* desc = [[MTLSamplerDescriptor alloc] init];
    const MTLSamplerMinMagFilter metalFilter = samplerDesc.filter == SamplerFilter::Nearest
            ? MTLSamplerMinMagFilterNearest : MTLSamplerMinMagFilterLinear;
    const MTLSamplerAddressMode metalAddress = samplerDesc.address == SamplerAddress::Repeat
            ? MTLSamplerAddressModeRepeat : MTLSamplerAddressModeClampToEdge;
    desc.minFilter = metalFilter;
    desc.magFilter = metalFilter;
    desc.mipFilter = MTLSamplerMipFilterNotMipmapped;
    desc.sAddressMode = metalAddress;
    desc.tAddressMode = metalAddress;
    desc.rAddressMode = metalAddress;
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

TextureBinding TexturePool::bind(const Texture& texture, std::uint32_t) { return bind(texture.handle()); }

TextureBinding TexturePool::bind(const VolumeTexture& volume, std::uint32_t) { return bind(volume.handle()); }

TexturePool::~TexturePool() {
    bindings_.clear();
    cinder::Gfx::MTL::release(sampler_);
}

}
