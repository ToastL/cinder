#pragma once

#include "gfx/rhi/Handles.hpp"

#include <volk.h>

namespace cinder::gfx::vk { class VkCtx; }

namespace cinder::gfx::rhi {



class TexturePool {
public:
    TexturePool(const cinder::gfx::vk::VkCtx& ctx, uint32_t maxSets, VkFilter filter);
    ~TexturePool();

    TexturePool(const TexturePool&) = delete;
    TexturePool& operator=(const TexturePool&) = delete;

    rhi::TextureBinding bind(VkImageView view) const;

private:
    const cinder::gfx::vk::VkCtx& ctx_;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

}
