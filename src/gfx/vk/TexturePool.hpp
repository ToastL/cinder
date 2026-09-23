#pragma once

#include "gfx/rhi/Handles.hpp"

#include <volk.h>

namespace cinder::gfx::rhi { class Ctx; }

namespace cinder::gfx::rhi {



class TexturePool {
public:
    TexturePool(const cinder::gfx::rhi::Ctx& ctx, uint32_t maxSets, VkFilter filter);
    ~TexturePool();

    TexturePool(const TexturePool&) = delete;
    TexturePool& operator=(const TexturePool&) = delete;

    rhi::TextureBinding bind(VkImageView view) const;

private:
    const cinder::gfx::rhi::Ctx& ctx_;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

}
