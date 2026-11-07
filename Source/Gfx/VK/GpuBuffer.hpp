#pragma once

#include "Gfx/RHI/Format.hpp"
#include "Gfx/RHI/Handles.hpp"

#include <volk.h>

#include <vk_mem_alloc.h>

#include <cstddef>

namespace cinder::Gfx::RHI { class Ctx; }

namespace cinder::Gfx::RHI {


class GpuBuffer {
public:
    GpuBuffer(const Ctx& ctx, std::uint64_t size, BufferUsage usage);
    ~GpuBuffer();

    GpuBuffer(const GpuBuffer&) = delete;
    GpuBuffer& operator=(const GpuBuffer&) = delete;
    GpuBuffer(GpuBuffer&& other) noexcept;
    GpuBuffer& operator=(GpuBuffer&& other) noexcept;

    void bindVertex(Commands cmd) const;
    void bindIndex(Commands cmd) const;

    VkBuffer handle() const { return handle_; }
    void* mapped() const { return mapped_; }
    VkDeviceSize size() const { return size_; }

private:
    void release();

    const cinder::Gfx::RHI::Ctx* ctx_ = nullptr;
    VkBuffer handle_ = VK_NULL_HANDLE;
    VmaAllocation allocation_ = nullptr;
    void* mapped_ = nullptr;
    VkDeviceSize size_ = 0;
};

}
