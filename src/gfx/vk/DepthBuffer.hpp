#pragma once

#include "gfx/vk/VkImages.hpp"

namespace cinder::gfx::vk {

class VkCtx;

class DepthBuffer {
public:
    DepthBuffer(const VkCtx& ctx, VkFormat format, uint32_t width, uint32_t height);
    ~DepthBuffer();

    DepthBuffer(const DepthBuffer&) = delete;
    DepthBuffer& operator=(const DepthBuffer&) = delete;

    static VkFormat chooseFormat(VkPhysicalDevice physicalDevice);

    VkImageView view() const { return view_; }

private:
    const VkCtx& ctx_;
    Allocated image_;
    VkImageView view_ = VK_NULL_HANDLE;
};

}
