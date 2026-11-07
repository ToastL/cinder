#pragma once

#include "Gfx/RHI/Handles.hpp"

#include "Gfx/VK/VkImages.hpp"

#include <string>

namespace cinder::Gfx::RHI {

class Texture {
public:

    static Texture load(const cinder::Gfx::RHI::Ctx& ctx, const std::string& path);
    static Texture white(const cinder::Gfx::RHI::Ctx& ctx);

    Texture(const cinder::Gfx::RHI::Ctx& ctx, const unsigned char* pixels,
            uint32_t width, uint32_t height);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& other) noexcept;

    VkImageView view() const { return view_; }
    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }

    TextureBinding binding() const { return binding_; }
    void setBinding(TextureBinding binding) { binding_ = binding; }

private:
    const cinder::Gfx::RHI::Ctx* ctx_ = nullptr;
    cinder::Gfx::VK::Allocated image_;
    VkImageView view_ = VK_NULL_HANDLE;
    TextureBinding binding_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

}
