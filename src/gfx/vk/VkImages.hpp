#pragma once

#include <volk.h>

#include <vk_mem_alloc.h>

namespace cinder::gfx::rhi { class Ctx; }

namespace cinder::gfx::vk {

struct Allocated {
    VkImage image = VK_NULL_HANDLE;
    VmaAllocation allocation = nullptr;
};

namespace images {

Allocated create(const rhi::Ctx& ctx, VkFormat format, uint32_t width, uint32_t height,
                 VkImageUsageFlags usage);
VkImageView view(const rhi::Ctx& ctx, VkImage image, VkFormat format, VkImageAspectFlags aspect);
VkSampler sampler(const rhi::Ctx& ctx, VkFilter filter);
VkFramebuffer framebuffer(const rhi::Ctx& ctx, VkRenderPass renderPass,
                          const VkImageView* attachments, uint32_t count,
                          uint32_t width, uint32_t height);

}

}
