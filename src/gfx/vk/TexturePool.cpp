#include "gfx/vk/TexturePool.hpp"

#include "gfx/vk/Bindings.hpp"
#include "gfx/vk/VkCtx.hpp"
#include "gfx/vk/VkDescriptors.hpp"
#include "gfx/vk/VkImages.hpp"

namespace cinder::gfx::rhi {

namespace descriptors = cinder::gfx::vk::descriptors;
namespace images = cinder::gfx::vk::images;

TexturePool::TexturePool(const cinder::gfx::vk::VkCtx& ctx, uint32_t maxSets, VkFilter filter) : ctx_(ctx) {
    pool_ = descriptors::pool(ctx, maxSets);
    sampler_ = images::sampler(ctx, filter);
}

rhi::TextureBinding TexturePool::bind(VkImageView view) const {
    const VkDescriptorSet set = descriptors::allocate(ctx_, pool_, ctx_.textureLayout());
    descriptors::writeCombinedImageSampler(ctx_, set, view, sampler_);
    return rhi::binding(set);
}

TexturePool::~TexturePool() {
    vkDestroySampler(ctx_.device(), sampler_, nullptr);
    vkDestroyDescriptorPool(ctx_.device(), pool_, nullptr);
}

}
