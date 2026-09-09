#pragma once

#include <volk.h>

namespace cinder::gfx::vk {

class VkCtx;

namespace descriptors {

VkDescriptorPool pool(const VkCtx& ctx, uint32_t maxSets);
VkDescriptorSet allocate(const VkCtx& ctx, VkDescriptorPool pool, VkDescriptorSetLayout layout);
void writeCombinedImageSampler(const VkCtx& ctx, VkDescriptorSet set,
                               VkImageView view, VkSampler sampler);

}

}
