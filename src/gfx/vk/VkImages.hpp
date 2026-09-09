#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

namespace cinder::gfx::vk {

class VkCtx;

struct Allocated {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
};

namespace images {

Allocated create(const VkCtx& ctx, VkFormat format, uint32_t width, uint32_t height,
                 VkImageUsageFlags usage);
VkImageView view(const VkCtx& ctx, VkImage image, VkFormat format, VkImageAspectFlags aspect);
VkSampler sampler(const VkCtx& ctx, VkFilter filter);
VkFramebuffer framebuffer(const VkCtx& ctx, VkRenderPass renderPass,
                          const VkImageView* attachments, uint32_t count,
                          uint32_t width, uint32_t height);

}

}
