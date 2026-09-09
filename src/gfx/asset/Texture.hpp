#pragma once

#include "gfx/vk/VkImages.hpp"

#include <string>

namespace cinder::gfx::asset {

class Texture {
public:
    static constexpr VkFormat FORMAT = VK_FORMAT_R8G8B8A8_SRGB;

    static Texture load(const cinder::gfx::vk::VkCtx& ctx, const std::string& path);
    static Texture white(const cinder::gfx::vk::VkCtx& ctx);

    Texture(const cinder::gfx::vk::VkCtx& ctx, const unsigned char* pixels,
            uint32_t width, uint32_t height);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;

    VkImageView view() const { return view_; }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

    VkDescriptorSet descriptorSet() const { return descriptorSet_; }
    void setDescriptorSet(VkDescriptorSet set) { descriptorSet_ = set; }

private:
    const cinder::gfx::vk::VkCtx* ctx_ = nullptr;
    cinder::gfx::vk::Allocated image_;
    VkImageView view_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet_ = VK_NULL_HANDLE;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

}
