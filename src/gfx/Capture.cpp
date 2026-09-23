#include "gfx/Capture.hpp"

#include "gfx/RenderTarget.hpp"
#include "gfx/vk/GpuBuffer.hpp"
#include "gfx/vk/VkCtx.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace cinder::gfx {

void captureTarget(const cinder::gfx::vk::VkCtx& ctx, const RenderTarget& target,
                   cinder::gfx::rhi::Format format, const std::string& path) {
    ctx.waitIdle();

    const uint32_t width = target.width();
    const uint32_t height = target.height();
    const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;

    cinder::gfx::vk::GpuBuffer staging(ctx, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);

    VkCommandBuffer cmd = ctx.beginSingleTime();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = target.image();
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.imageExtent = {width, height, 1};
    vkCmdCopyImageToBuffer(cmd, target.image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           staging.handle(), 1, &copy);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                         1, &barrier);

    ctx.endSingleTime(cmd);

    writeCapture(path, width, height, staging.mapped(), format);
}

void writeCapture(const std::string& path, uint32_t width, uint32_t height, const void* data,
                  cinder::gfx::rhi::Format format) {
    const std::size_t size = static_cast<std::size_t>(width) * height * 4;
    std::vector<unsigned char> pixels(size);
    std::memcpy(pixels.data(), data, size);

    if (cinder::gfx::rhi::isBgra(format)) {
        for (std::size_t i = 0; i < pixels.size(); i += 4) std::swap(pixels[i], pixels[i + 2]);
    }

    stbi_write_png(path.c_str(), static_cast<int>(width), static_cast<int>(height), 4,
                   pixels.data(), static_cast<int>(width) * 4);
}

}
