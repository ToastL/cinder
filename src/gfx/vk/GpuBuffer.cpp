#include "gfx/vk/GpuBuffer.hpp"

#include "gfx/vk/Commands.hpp"
#include "gfx/vk/VkCtx.hpp"
#include "gfx/vk/VkUtil.hpp"

#include <utility>

namespace cinder::gfx::rhi {

using cinder::gfx::vk::check;

GpuBuffer::GpuBuffer(const cinder::gfx::vk::VkCtx& ctx, VkDeviceSize size, VkBufferUsageFlags usage,
                     bool hostVisible)
    : ctx_(&ctx), size_(size) {
    VkBufferCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    info.size = size;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo alloc{};
    alloc.usage = VMA_MEMORY_USAGE_AUTO;
    if (hostVisible) {
        alloc.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                | VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VmaAllocationInfo result{};
    check(vmaCreateBuffer(ctx.allocator(), &info, &alloc, &handle_, &allocation_, &result),
          "vmaCreateBuffer");

    if (hostVisible) mapped_ = result.pMappedData;
}

GpuBuffer::GpuBuffer(GpuBuffer&& other) noexcept
    : ctx_(other.ctx_), handle_(other.handle_), allocation_(other.allocation_),
      mapped_(other.mapped_), size_(other.size_) {
    other.handle_ = VK_NULL_HANDLE;
    other.allocation_ = nullptr;
    other.mapped_ = nullptr;
}

GpuBuffer& GpuBuffer::operator=(GpuBuffer&& other) noexcept {
    if (this != &other) {
        release();
        ctx_ = other.ctx_;
        handle_ = std::exchange(other.handle_, VK_NULL_HANDLE);
        allocation_ = std::exchange(other.allocation_, nullptr);
        mapped_ = std::exchange(other.mapped_, nullptr);
        size_ = other.size_;
    }
    return *this;
}

void GpuBuffer::bindVertex(Commands cmd) const {
    const VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(unwrap(cmd), 0, 1, &handle_, &offset);
}

void GpuBuffer::bindIndex(Commands cmd) const {
    vkCmdBindIndexBuffer(unwrap(cmd), handle_, 0, VK_INDEX_TYPE_UINT32);
}

GpuBuffer::~GpuBuffer() { release(); }

void GpuBuffer::release() {
    if (handle_ == VK_NULL_HANDLE) return;
    vmaDestroyBuffer(ctx_->allocator(), handle_, allocation_);
    handle_ = VK_NULL_HANDLE;
    allocation_ = nullptr;
    mapped_ = nullptr;
}

}
