#include "gfx/vk/GlyphPages.hpp"

#include "gfx/vk/Commands.hpp"
#include "gfx/vk/GpuBuffer.hpp"
#include "gfx/vk/Ctx.hpp"

namespace cinder::gfx::rhi {
namespace images = cinder::gfx::vk::images;

namespace {

constexpr VkFormat PAGE_FORMAT = VK_FORMAT_R8_UNORM;

void barrier(VkCommandBuffer cmd, VkImage image, VkImageLayout from, VkImageLayout to,
             VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage,
             VkPipelineStageFlags dstStage) {
    VkImageMemoryBarrier info{};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    info.oldLayout = from;
    info.newLayout = to;
    info.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    info.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    info.image = image;
    info.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    info.srcAccessMask = srcAccess;
    info.dstAccessMask = dstAccess;
    vkCmdPipelineBarrier(cmd, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &info);
}

}

GlyphPages::GlyphPages(const cinder::gfx::rhi::Ctx& ctx, std::uint32_t size)
    : ctx_(ctx), size_(size) {}

GlyphPages::Page& GlyphPages::page(std::size_t index) {
    while (pages_.size() <= index) {
        Page& created = pages_.emplace_back();
        created.image = images::create(ctx_, PAGE_FORMAT, size_, size_,
                                       VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
        created.view = images::view(ctx_, created.image.image, PAGE_FORMAT,
                                    VK_IMAGE_ASPECT_COLOR_BIT);
        created.pool = std::make_unique<TexturePool>(ctx_, 1, SamplerFilter::Linear);
        created.set = created.pool->bind(created.view);
    }
    return pages_[index];
}

TextureBinding GlyphPages::binding(std::size_t index) { return page(index).set; }

bool GlyphPages::uploaded(std::size_t index) { return page(index).uploaded; }

void GlyphPages::forgetUploads() {
    for (Page& existing : pages_) existing.uploaded = false;
}

void GlyphPages::upload(Uploads cmd, const GpuBuffer& staging,
                        const std::vector<GlyphRegion>& regions) {
    for (const GlyphRegion& region : regions) {
        Page& target = page(region.page);

        barrier(unwrap(cmd), target.image.image,
                target.uploaded ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                                : VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

        VkBufferImageCopy copy{};
        copy.bufferOffset = region.offset;
        copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
        copy.imageOffset = {region.x, region.y, 0};
        copy.imageExtent = {static_cast<std::uint32_t>(region.width),
                            static_cast<std::uint32_t>(region.height), 1};
        vkCmdCopyBufferToImage(unwrap(cmd), staging.handle(), target.image.image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        barrier(unwrap(cmd), target.image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        target.uploaded = true;
    }
}

GlyphPages::~GlyphPages() {
    for (Page& existing : pages_) {
        existing.pool.reset();
        vkDestroyImageView(ctx_.device(), existing.view, nullptr);
        vmaDestroyImage(ctx_.allocator(), existing.image.image, existing.image.allocation);
    }
}

}
