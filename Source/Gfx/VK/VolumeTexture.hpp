#pragma once

#include "Gfx/RHI/Format.hpp"
#include "Gfx/RHI/Handles.hpp"
#include "Gfx/VK/VkImages.hpp"

#include <cstdint>

namespace cinder::Gfx::RHI { class Ctx; }

namespace cinder::Gfx::RHI {

class VolumeTexture {
public:
    VolumeTexture(const Ctx& ctx, const void* data, std::uint32_t width, std::uint32_t height,
                  std::uint32_t depth, Format format);
    ~VolumeTexture();

    VolumeTexture(const VolumeTexture&) = delete;
    VolumeTexture& operator=(const VolumeTexture&) = delete;
    VolumeTexture(VolumeTexture&& other) noexcept;

    VkImageView view() const { return view_; }
    std::uint32_t width() const { return width_; }
    std::uint32_t height() const { return height_; }
    std::uint32_t depth() const { return depth_; }

    TextureBinding binding() const { return binding_; }
    void setBinding(TextureBinding binding) { binding_ = binding; }

private:
    const Ctx* ctx_ = nullptr;
    cinder::Gfx::VK::Allocated image_;
    VkImageView view_ = VK_NULL_HANDLE;
    TextureBinding binding_;
    std::uint32_t width_ = 0;
    std::uint32_t height_ = 0;
    std::uint32_t depth_ = 0;
};

}
