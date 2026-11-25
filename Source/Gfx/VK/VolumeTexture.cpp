#include "Gfx/VK/VolumeTexture.hpp"

#include "Gfx/VK/Ctx.hpp"
#include "Gfx/VK/Formats.hpp"
#include "Gfx/VK/GpuBuffer.hpp"

#include <cstring>
#include <stdexcept>
#include <utility>

namespace cinder::Gfx::RHI {

namespace images = cinder::Gfx::VK::images;

namespace {

std::uint32_t bytesPerVoxel(Format format) {
    switch (format) {
        case Format::R8Unorm: return 1;
        case Format::RGBA8Unorm:
        case Format::RGBA8Srgb:
        case Format::BGRA8Unorm:
        case Format::BGRA8Srgb: return 4;
        case Format::RGBA16Sfloat: return 8;
        default: return 0;
    }
}

}

VolumeTexture::VolumeTexture(const Ctx& ctx, const void* data, std::uint32_t width,
                             std::uint32_t height, std::uint32_t depth, Format format)
    : ctx_(&ctx), width_(width), height_(height), depth_(depth) {
    const VkFormat vkFormat = toVk(format);
    const std::uint32_t voxelBytes = bytesPerVoxel(format);
    if (vkFormat == VK_FORMAT_UNDEFINED || voxelBytes == 0) {
        throw std::runtime_error("VolumeTexture: unsupported format");
    }

    const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * depth * voxelBytes;

    GpuBuffer staging(ctx, size, BufferUsage::TransferSrc);
    std::memcpy(staging.mapped(), data, static_cast<std::size_t>(size));

    image_ = images::create3D(ctx, vkFormat, width, height, depth,
                              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

    VkCommandBuffer cmd = ctx.beginSingleTime();

    images::barrier(cmd, image_.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT);

    VkBufferImageCopy copy{};
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageOffset = {0, 0, 0};
    copy.imageExtent = {width, height, depth};

    vkCmdCopyBufferToImage(cmd, staging.handle(), image_.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    images::barrier(cmd, image_.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    ctx.endSingleTime(cmd);

    view_ = images::view3D(ctx, image_.image, vkFormat, VK_IMAGE_ASPECT_COLOR_BIT);
}

VolumeTexture::VolumeTexture(VolumeTexture&& other) noexcept
    : ctx_(other.ctx_), image_(other.image_), view_(other.view_), binding_(other.binding_),
      width_(other.width_), height_(other.height_), depth_(other.depth_) {
    other.image_ = {};
    other.view_ = VK_NULL_HANDLE;
}

VolumeTexture::~VolumeTexture() {
    if (ctx_ == nullptr) return;
    if (view_ != VK_NULL_HANDLE) vkDestroyImageView(ctx_->device(), view_, nullptr);
    if (image_.image != VK_NULL_HANDLE) {
        vmaDestroyImage(ctx_->allocator(), image_.image, image_.allocation);
    }
}

}
