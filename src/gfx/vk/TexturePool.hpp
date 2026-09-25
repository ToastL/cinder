#pragma once

#include "gfx/rhi/Handles.hpp"
#include "gfx/rhi/Format.hpp"

#include <volk.h>

namespace cinder::gfx::rhi {

class Texture; class Ctx; }

namespace cinder::gfx::rhi {



class TexturePool {
public:
    TexturePool(const cinder::gfx::rhi::Ctx& ctx, uint32_t maxSets, SamplerFilter filter);
    ~TexturePool();

    TexturePool(const TexturePool&) = delete;
    TexturePool& operator=(const TexturePool&) = delete;

    TextureBinding bind(VkImageView view) const;
    TextureBinding bind(const Texture& texture) const;

private:
    const cinder::gfx::rhi::Ctx& ctx_;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

}
