#include "gfx/vk/Swapchain.hpp"

#include "gfx/vk/VkCtx.hpp"
#include "gfx/vk/VkImages.hpp"
#include "gfx/vk/VkUtil.hpp"
#include "platform/Window.hpp"

#include <algorithm>

namespace cinder::gfx::vk {

Swapchain::Swapchain(const VkCtx& ctx, const cinder::platform::Window& window) : ctx_(ctx) {
    VkSurfaceCapabilitiesKHR caps{};
    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice(), ctx.surface(), &caps),
          "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice(), ctx.surface(), &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice(), ctx.surface(), &formatCount,
                                         formats.data());

    VkSurfaceFormatKHR chosen = formats.front();
    for (const VkSurfaceFormatKHR& candidate : formats) {
        if (candidate.format == VK_FORMAT_B8G8R8A8_SRGB
                && candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            chosen = candidate;
            break;
        }
    }
    format_ = chosen.format;

    uint32_t modeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physicalDevice(), ctx.surface(), &modeCount,
                                              nullptr);
    std::vector<VkPresentModeKHR> modes(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.physicalDevice(), ctx.surface(), &modeCount,
                                              modes.data());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (VkPresentModeKHR mode : modes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) presentMode = mode;
    }

    if (caps.currentExtent.width != 0xFFFFFFFF) {
        extent_ = caps.currentExtent;
    } else {
        extent_.width = std::clamp(static_cast<uint32_t>(window.width()),
                                   caps.minImageExtent.width, caps.maxImageExtent.width);
        extent_.height = std::clamp(static_cast<uint32_t>(window.height()),
                                    caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    uint32_t requested = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && requested > caps.maxImageCount) requested = caps.maxImageCount;

    VkSwapchainCreateInfoKHR info{};
    info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    info.surface = ctx.surface();
    info.minImageCount = requested;
    info.imageFormat = format_;
    info.imageColorSpace = chosen.colorSpace;
    info.imageExtent = extent_;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = presentMode;
    info.clipped = VK_TRUE;
    info.oldSwapchain = VK_NULL_HANDLE;

    const uint32_t families[] = {ctx.graphicsFamily(), ctx.presentFamily()};
    if (ctx.graphicsFamily() != ctx.presentFamily()) {
        info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        info.queueFamilyIndexCount = 2;
        info.pQueueFamilyIndices = families;
    } else {
        info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    check(vkCreateSwapchainKHR(ctx.device(), &info, nullptr, &handle_), "vkCreateSwapchainKHR");

    uint32_t count = 0;
    vkGetSwapchainImagesKHR(ctx.device(), handle_, &count, nullptr);
    images_.resize(count);
    vkGetSwapchainImagesKHR(ctx.device(), handle_, &count, images_.data());

    views_.reserve(count);
    for (VkImage image : images_) {
        views_.push_back(images::view(ctx, image, format_, VK_IMAGE_ASPECT_COLOR_BIT));
    }
}

void Swapchain::createFramebuffers(VkRenderPass renderPass) {
    destroyFramebuffers();
    framebuffers_.reserve(views_.size());
    for (VkImageView view : views_) {
        framebuffers_.push_back(
                images::framebuffer(ctx_, renderPass, &view, 1, extent_.width, extent_.height));
    }
}

void Swapchain::destroyFramebuffers() {
    for (VkFramebuffer framebuffer : framebuffers_) {
        vkDestroyFramebuffer(ctx_.device(), framebuffer, nullptr);
    }
    framebuffers_.clear();
}

Swapchain::~Swapchain() {
    destroyFramebuffers();
    for (VkImageView view : views_) vkDestroyImageView(ctx_.device(), view, nullptr);
    if (handle_ != VK_NULL_HANDLE) vkDestroySwapchainKHR(ctx_.device(), handle_, nullptr);
}

}
