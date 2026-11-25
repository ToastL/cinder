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

TextureBinding TexturePool::bind(VkImageView view, VkImageLayout layout, uint32_t slot) const {
    const VkDescriptorSet set = descriptors::allocate(ctx_, pool_, ctx_.textureLayout(slot));
    descriptors::writeCombinedImageSampler(ctx_, set, view, sampler_, layout, slot);
    return binding(set);
}

TextureBinding TexturePool::bind(const Texture& texture, uint32_t slot) const {
    return bind(texture.view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, slot);
}

TextureBinding TexturePool::bind(const VolumeTexture& volume, uint32_t slot) const {
    return bind(volume.view(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, slot);
}

TexturePool::~TexturePool() {
    vkDestroySampler(ctx_.device(), sampler_, nullptr);
    vkDestroyDescriptorPool(ctx_.device(), pool_, nullptr);
}

}
