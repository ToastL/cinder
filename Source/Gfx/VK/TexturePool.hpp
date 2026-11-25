#pragma once

#include "Gfx/RHI/Handles.hpp"
#include "Gfx/RHI/Format.hpp"

#include <volk.h>

namespace cinder::Gfx::RHI {

class Texture; class VolumeTexture; class Ctx; }

namespace cinder::Gfx::RHI {



class TexturePool {
public:
    TexturePool(const cinder::Gfx::RHI::Ctx& ctx, uint32_t maxSets, SamplerFilter filter);
    TexturePool(const cinder::Gfx::RHI::Ctx& ctx, uint32_t maxSets, SamplerDesc sampler);
    ~TexturePool();

    TexturePool(const TexturePool&) = delete;
    TexturePool& operator=(const TexturePool&) = delete;

    TextureBinding bind(VkImageView view, VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        uint32_t slot = 0) const;
    TextureBinding bind(const Texture& texture, uint32_t slot = 0) const;
    TextureBinding bind(const VolumeTexture& volume, uint32_t slot = 0) const;

private:
    const cinder::Gfx::RHI::Ctx& ctx_;
    VkDescriptorPool pool_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
};

}
