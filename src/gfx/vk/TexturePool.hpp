#pragma once

#include <volk.h>

namespace cinder::gfx::vk {

class VkCtx;

class TexturePool {
public:
    TexturePool(const VkCtx& ctx, uint32_t maxSets, VkFilter filter);
    ~TexturePool();

    TexturePool(const TexturePool&) = delete;
    TexturePool& operator=(const TexturePool&) = delete;

    VkDescriptorSet bind(VkImageView view) const;

private:
    const VkCtx& ctx_;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

}
