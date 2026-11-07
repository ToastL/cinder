#pragma once

#include "Gfx/VK/VkImages.hpp"

namespace cinder::Gfx::RHI { class Ctx; }

namespace cinder::Gfx::VK {

class DepthBuffer {
public:
    DepthBuffer(const RHI::Ctx& ctx, VkFormat format, uint32_t width, uint32_t height);
    ~DepthBuffer();

    DepthBuffer(const DepthBuffer&) = delete;
    DepthBuffer& operator=(const DepthBuffer&) = delete;

    static VkFormat chooseFormat(VkPhysicalDevice physicalDevice);

    VkImageView view() const { return view_; }

private:
    const RHI::Ctx& ctx_;
    Allocated image_;
    VkImageView view_ = VK_NULL_HANDLE;
};

}
