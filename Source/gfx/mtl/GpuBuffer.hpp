#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Handles.hpp"

#include <cstdint>

namespace cinder::gfx::rhi {

class Ctx;

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

    void* handle() const { return handle_; }
    void* mapped() const { return mapped_; }
    std::uint64_t size() const { return size_; }

private:
    void release();

    void* handle_ = nullptr;
    void* mapped_ = nullptr;
    std::uint64_t size_ = 0;
};

}
