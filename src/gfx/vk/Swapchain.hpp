#pragma once

#include "gfx/rhi/Format.hpp"

#include <volk.h>

#include <vector>

namespace cinder::platform { class Window; }

namespace cinder::gfx::vk {

class VkCtx;

class Swapchain {
public:
    Swapchain(const VkCtx& ctx, const cinder::platform::Window& window, VkImageUsageFlags extraUsage = 0);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    static bool supports(const VkCtx& ctx, VkImageUsageFlags usage);

    void createFramebuffers(VkRenderPass renderPass);

    VkSwapchainKHR handle() const { return handle_; }
    uint32_t imageCount() const { return static_cast<uint32_t>(images_.size()); }
    VkFramebuffer framebuffer(uint32_t index) const { return framebuffers_[index]; }
    VkImage image(uint32_t index) const { return images_[index]; }
    rhi::Format format() const;
    uint32_t width() const { return extent_.width; }
    uint32_t height() const { return extent_.height; }

private:
    void destroyFramebuffers();

    const VkCtx& ctx_;
    VkSwapchainKHR handle_ = VK_NULL_HANDLE;
    std::vector<VkImage> images_;
    std::vector<VkImageView> views_;
    std::vector<VkFramebuffer> framebuffers_;
    VkFormat format_ = VK_FORMAT_UNDEFINED;
    VkExtent2D extent_{};
};

}
