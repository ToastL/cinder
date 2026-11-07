#include "gfx/vk/Texture.hpp"

#include "gfx/rhi/Image.hpp"
#include "gfx/vk/Ctx.hpp"
#include "gfx/vk/GpuBuffer.hpp"

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

    images::barrier(cmd, image_.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

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

    images::barrier(cmd, image_.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    ctx.endSingleTime(cmd);

    view_ = images::view(ctx, image_.image, FORMAT, VK_IMAGE_ASPECT_COLOR_BIT);
}

Texture Texture::load(const rhi::Ctx& ctx, const std::string& path) {
    const Image image = loadImage(path);
    return Texture(ctx, image.pixels.data(), image.width, image.height);
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
