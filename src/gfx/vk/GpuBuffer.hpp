#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Handles.hpp"

#include <volk.h>

#include <vk_mem_alloc.h>

#include <cstddef>

namespace cinder::gfx::rhi { class Ctx; }

namespace cinder::gfx::rhi {


class GpuBuffer {
public:
    GpuBuffer(const Ctx& ctx, std::uint64_t size, BufferUsage usage, bool hostVisible);
    ~GpuBuffer();

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;
    GpuBuffer(GpuBuffer&& other) noexcept;
    GpuBuffer& operator=(GpuBuffer&& other) noexcept;

    void bindVertex(Commands cmd) const;
    void bindIndex(Commands cmd) const;

    VkBuffer handle() const { return handle_; }
    VmaAllocation allocation() const { return allocation_; }
    void* mapped() const { return mapped_; }
    VkDeviceSize size() const { return size_; }

private:
    void release();

    const cinder::gfx::rhi::Ctx* ctx_ = nullptr;
    VkBuffer handle_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = nullptr;
    void* mapped_ = nullptr;
    VkDeviceSize size_ = 0;
};

}
