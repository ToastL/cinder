#pragma once

#include <volk.h>

namespace cinder::gfx::rhi { class Ctx; }

namespace cinder::gfx::vk {

namespace descriptors {

VkDescriptorPool pool(const rhi::Ctx& ctx, uint32_t maxSets);
VkDescriptorSet allocate(const rhi::Ctx& ctx, VkDescriptorPool pool, VkDescriptorSetLayout layout);
void writeCombinedImageSampler(const rhi::Ctx& ctx, VkDescriptorSet set,
                               VkImageView view, VkSampler sampler);

}

}
