#pragma once

#include "Gfx/RHI/Handles.hpp"
#include "Gfx/RHI/Format.hpp"

#include <volk.h>

namespace cinder::Gfx::RHI {

class Texture; class Ctx; }

namespace cinder::Gfx::RHI {



class TexturePool {
public:
    TexturePool(const cinder::Gfx::RHI::Ctx& ctx, uint32_t maxSets, SamplerFilter filter);
    ~TexturePool();

    TexturePool(const TexturePool&) = delete;
    TexturePool& operator=(const TexturePool&) = delete;

    TextureBinding bind(VkImageView view) const;
    TextureBinding bind(const Texture& texture) const;

private:
    const cinder::Gfx::RHI::Ctx& ctx_;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

}
