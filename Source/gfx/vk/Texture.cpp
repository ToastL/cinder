#include "gfx/vk/Texture.hpp"

#include "gfx/vk/GpuBuffer.hpp"
#include "gfx/vk/Ctx.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>
#include <stdexcept>
#include <utility>

namespace cinder::gfx::rhi {
namespace {

constexpr VkFormat FORMAT = VK_FORMAT_R8G8B8A8_SRGB;

}

using cinder::gfx::rhi::GpuBuffer;
using cinder::gfx::rhi::Ctx;
namespace images = cinder::gfx::vk::images;

Texture::Texture(const rhi::Ctx& ctx, const unsigned char* pixels, uint32_t width, uint32_t height)
    : ctx_(&ctx), width_(width), height_(height) {
    const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;

    GpuBuffer staging(ctx, size, BufferUsage::TransferSrc, true);
    std::memcpy(staging.mapped(), pixels, static_cast<std::size_t>(size));

    image_ = images::create(ctx, FORMAT, width, height,
                            VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    VkCommandBuffer cmd = ctx.beginSingleTime();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image_.image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy copy{};
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageOffset = {0, 0, 0};
    copy.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cmd, staging.handle(), image_.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);

    ctx.endSingleTime(cmd);

    view_ = images::view(ctx, image_.image, FORMAT, VK_IMAGE_ASPECT_COLOR_BIT);
}

Texture Texture::load(const rhi::Ctx& ctx, const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        throw std::runtime_error("Failed to load " + path + ": " + stbi_failure_reason());
    }

    Texture texture(ctx, pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    stbi_image_free(pixels);
    return texture;
}

Texture Texture::white(const rhi::Ctx& ctx) {
    const unsigned char pixels[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    return Texture(ctx, pixels, 1, 1);
}

Texture::Texture(Texture&& other) noexcept
    : ctx_(other.ctx_), image_(other.image_), view_(other.view_),
      binding_(other.binding_), width_(other.width_), height_(other.height_) {
    other.image_ = {};
    other.view_ = VK_NULL_HANDLE;
}

Texture::~Texture() {
    if (view_ != VK_NULL_HANDLE) vkDestroyImageView(ctx_->device(), view_, nullptr);
    if (image_.image != VK_NULL_HANDLE) {
        vmaDestroyImage(ctx_->allocator(), image_.image, image_.allocation);
    }
}

}
