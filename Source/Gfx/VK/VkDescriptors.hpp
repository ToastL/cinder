#pragma once

#include <volk.h>

namespace cinder::Gfx::RHI { class Ctx; }

namespace cinder::Gfx::VK {

namespace descriptors {

VkDescriptorPool pool(const RHI::Ctx& ctx, uint32_t maxSets);
VkDescriptorSet allocate(const RHI::Ctx& ctx, VkDescriptorPool pool, VkDescriptorSetLayout layout);
void writeCombinedImageSampler(const RHI::Ctx& ctx, VkDescriptorSet set,
                               VkImageView view, VkSampler sampler);

}

}
