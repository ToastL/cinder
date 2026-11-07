#pragma once

#include "Gfx/RHI/Format.hpp"

#include <volk.h>

#include <vector>

namespace cinder::Platform { class Window; }

namespace cinder::Gfx::RHI { class Ctx; }

namespace cinder::Gfx::VK {

class Swapchain {
public:
    Swapchain(const RHI::Ctx& ctx, const cinder::Platform::Window& window, VkImageUsageFlags extraUsage = 0);
    ~Swapchain();

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    static bool supports(const RHI::Ctx& ctx, VkImageUsageFlags usage);

    void createFramebuffers(VkRenderPass renderPass);

    VkSwapchainKHR handle() const { return handle_; }
    uint32_t imageCount() const { return static_cast<uint32_t>(images_.size()); }
    VkFramebuffer framebuffer(uint32_t index) const { return framebuffers_[index]; }
    VkImage image(uint32_t index) const { return images_[index]; }
    RHI::Format format() const;
    uint32_t width() const { return extent_.width; }
    uint32_t height() const { return extent_.height; }
    VkImageUsageFlags usage() const { return usage_; }

private:
    void destroyFramebuffers();

    const RHI::Ctx& ctx_;
    VkSwapchainKHR handle_ = VK_NULL_HANDLE;
    std::vector<VkImage> images_;
    std::vector<VkImageView> views_;
    std::vector<VkFramebuffer> framebuffers_;
    VkFormat format_ = VK_FORMAT_UNDEFINED;
    VkExtent2D extent_{};
    VkImageUsageFlags usage_ = 0;
};

}
