#include "Gfx/VK/Swapchain.hpp"

#include "Gfx/VK/Ctx.hpp"
#include "Gfx/VK/VkImages.hpp"
#include "Gfx/VK/VkUtil.hpp"
#include "Gfx/VK/Formats.hpp"
#include "Platform/Window.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace cinder::Gfx::VK {
namespace {

constexpr uint32_t UNDEFINED_EXTENT = 0xFFFFFFFF;

VkSurfaceFormatKHR pickFormat(const std::vector<VkSurfaceFormatKHR>& formats) {
    const VkFormat preferred[] = {VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_UNORM,
                                  VK_FORMAT_R8G8B8A8_UNORM};
    for (const VkFormat wanted : preferred) {
        for (const VkSurfaceFormatKHR& candidate : formats) {
            if (candidate.format == wanted && candidate.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return candidate;
            }
        }
    }
    for (const VkSurfaceFormatKHR& candidate : formats) {
        if (RHI::knownVk(candidate.format)) return candidate;
    }
    throw std::runtime_error("the surface offers no colour format the renderer supports");
}

}

Swapchain::Swapchain(const RHI::Ctx& ctx, const cinder::Platform::Window& window, VkImageUsageFlags extraUsage)
    : ctx_(ctx) {
    VkSurfaceCapabilitiesKHR caps{};
    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice(), ctx.surface(), &caps),
          "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

    uint32_t formatCount = 0;
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice(), ctx.surface(), &formatCount, nullptr),
          "vkGetPhysicalDeviceSurfaceFormatsKHR");
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    check(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.physicalDevice(), ctx.surface(), &formatCount, formats.data()),
          "vkGetPhysicalDeviceSurfaceFormatsKHR");

    const VkSurfaceFormatKHR chosen = pickFormat(formats);
    format_ = chosen.format;

    if (caps.currentExtent.width != UNDEFINED_EXTENT) {
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
    usage_ = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | (extraUsage & caps.supportedUsageFlags);
    info.imageUsage = usage_;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
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
    check(vkGetSwapchainImagesKHR(ctx.device(), handle_, &count, nullptr), "vkGetSwapchainImagesKHR");
    images_.resize(count);
    check(vkGetSwapchainImagesKHR(ctx.device(), handle_, &count, images_.data()), "vkGetSwapchainImagesKHR");

    views_.reserve(count);
    for (VkImage image : images_) {
        views_.push_back(images::view(ctx, image, format_, VK_IMAGE_ASPECT_COLOR_BIT));
    }
}

bool Swapchain::supports(const RHI::Ctx& ctx, VkImageUsageFlags usage) {
    VkSurfaceCapabilitiesKHR caps{};
    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.physicalDevice(), ctx.surface(), &caps),
          "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    return (caps.supportedUsageFlags & usage) == usage;
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


RHI::Format Swapchain::format() const { return RHI::fromVk(format_); }

}
