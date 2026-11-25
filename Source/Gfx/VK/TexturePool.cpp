#include "Gfx/VK/TexturePool.hpp"

#include "Gfx/VK/Bindings.hpp"
#include "Gfx/VK/Texture.hpp"
#include "Gfx/VK/VolumeTexture.hpp"
#include "Gfx/VK/Ctx.hpp"
#include "Gfx/VK/VkDescriptors.hpp"
#include "Gfx/VK/VkImages.hpp"

namespace cinder::Gfx::RHI {

namespace descriptors = cinder::Gfx::VK::descriptors;
namespace images = cinder::Gfx::VK::images;

TexturePool::TexturePool(const cinder::Gfx::RHI::Ctx& ctx, uint32_t maxSets,
                         SamplerFilter filter)
    : TexturePool(ctx, maxSets, SamplerDesc{filter, SamplerAddress::ClampToEdge}) {}

TexturePool::TexturePool(const cinder::Gfx::RHI::Ctx& ctx, uint32_t maxSets,
                         SamplerDesc desc) : ctx_(ctx) {
    pool_ = descriptors::pool(ctx, maxSets);
    const VkFilter filter = desc.filter == SamplerFilter::Nearest ? VK_FILTER_NEAREST
                                                                   : VK_FILTER_LINEAR;
    const VkSamplerAddressMode address = desc.address == SamplerAddress::Repeat
                                                 ? VK_SAMPLER_ADDRESS_MODE_REPEAT
                                                 : VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ = images::sampler(ctx, filter, address);
}

TextureBinding TexturePool::bind(VkImageView view) const {
    const VkDescriptorSet set = descriptors::allocate(ctx_, pool_, ctx_.textureLayout());
    descriptors::writeCombinedImageSampler(ctx_, set, view, sampler_);
    return binding(set);
}

TextureBinding TexturePool::bind(const Texture& texture) const {
    return bind(texture.view());
}

TextureBinding TexturePool::bind(const VolumeTexture& volume) const {
    return bind(volume.view());
}

TexturePool::~TexturePool() {
    vkDestroySampler(ctx_.device(), sampler_, nullptr);
    vkDestroyDescriptorPool(ctx_.device(), pool_, nullptr);
}

}
