#include "gfx/vk/VkImages.hpp"

#include "gfx/vk/Ctx.hpp"
#include "gfx/vk/VkUtil.hpp"

namespace cinder::gfx::vk::images {

Allocated create(const rhi::Ctx& ctx, VkFormat format, uint32_t width, uint32_t height,
                 VkImageUsageFlags usage) {
    VkImageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = {width, height, 1};
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo alloc{};
    alloc.usage = VMA_MEMORY_USAGE_AUTO;

    Allocated out;
    check(vmaCreateImage(ctx.allocator(), &info, &alloc, &out.image, &out.allocation, nullptr),
          "vmaCreateImage");
    return out;
}

VkImageView view(const rhi::Ctx& ctx, VkImage image, VkFormat format, VkImageAspectFlags aspect) {
    VkImageViewCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image = image;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = format;
    info.subresourceRange.aspectMask = aspect;
    info.subresourceRange.baseMipLevel = 0;
    info.subresourceRange.levelCount = 1;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount = 1;

    VkImageView handle = VK_NULL_HANDLE;
    check(vkCreateImageView(ctx.device(), &info, nullptr, &handle), "vkCreateImageView");
    return handle;
}

VkSampler sampler(const rhi::Ctx& ctx, VkFilter filter) {
    VkSamplerCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = filter;
    info.minFilter = filter;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    info.anisotropyEnable = VK_FALSE;
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;
    info.compareEnable = VK_FALSE;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;

    VkSampler handle = VK_NULL_HANDLE;
    check(vkCreateSampler(ctx.device(), &info, nullptr, &handle), "vkCreateSampler");
    return handle;
}

VkFramebuffer framebuffer(const rhi::Ctx& ctx, VkRenderPass renderPass,
                          const VkImageView* attachments, uint32_t count,
                          uint32_t width, uint32_t height) {
    VkFramebufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    info.renderPass = renderPass;
    info.attachmentCount = count;
    info.pAttachments = attachments;
    info.width = width;
    info.height = height;
    info.layers = 1;

    VkFramebuffer handle = VK_NULL_HANDLE;
    check(vkCreateFramebuffer(ctx.device(), &info, nullptr, &handle), "vkCreateFramebuffer");
    return handle;
}

}
