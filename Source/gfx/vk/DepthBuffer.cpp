#include "gfx/vk/DepthBuffer.hpp"

#include "gfx/vk/Ctx.hpp"

#include <stdexcept>

namespace cinder::gfx::vk {

DepthBuffer::DepthBuffer(const rhi::Ctx& ctx, VkFormat format, uint32_t width, uint32_t height)
    : ctx_(ctx) {
    image_ = images::create(ctx, format, width, height, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    view_ = images::view(ctx, image_.image, format, VK_IMAGE_ASPECT_DEPTH_BIT);
}

DepthBuffer::~DepthBuffer() {
    vkDestroyImageView(ctx_.device(), view_, nullptr);
    vmaDestroyImage(ctx_.allocator(), image_.image, image_.allocation);
}

VkFormat DepthBuffer::chooseFormat(VkPhysicalDevice physicalDevice) {
    const VkFormat candidates[] = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT,
    };

    for (VkFormat format : candidates) {
        VkFormatProperties properties{};
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);
        if ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0) {
            return format;
        }
    }
    throw std::runtime_error("No supported depth format");
}

}
