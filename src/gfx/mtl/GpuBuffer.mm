#include "gfx/mtl/GpuBuffer.hpp"

#include "gfx/mtl/Commands.hpp"
#include "gfx/mtl/Ctx.hpp"
#include "gfx/mtl/MetalUtil.hpp"

#include <utility>

namespace cinder::gfx::rhi {

GpuBuffer::GpuBuffer(const Ctx& ctx, std::uint64_t size, BufferUsage, bool)
    : size_(size) {
    id<MTLDevice> device = cinder::gfx::mtl::bridge<id<MTLDevice>>(ctx.device());
    id<MTLBuffer> buffer = [device newBufferWithLength:static_cast<NSUInteger>(size)
                                             options:MTLResourceStorageModeShared];
    if (buffer == nil) throw cinder::gfx::mtl::error("Could not create Metal buffer");
    handle_ = cinder::gfx::mtl::retain(buffer);
    mapped_ = buffer.contents;
}

GpuBuffer::GpuBuffer(GpuBuffer&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr)),
      mapped_(std::exchange(other.mapped_, nullptr)), size_(other.size_) {}

GpuBuffer& GpuBuffer::operator=(GpuBuffer&& other) noexcept {
    if (this == &other) return *this;
    release();
    handle_ = std::exchange(other.handle_, nullptr);
    mapped_ = std::exchange(other.mapped_, nullptr);
    size_ = other.size_;
    return *this;
}

void GpuBuffer::bindVertex(Commands cmd) const {
    auto& state = unwrap(cmd);
    id<MTLRenderCommandEncoder> encoder =
            cinder::gfx::mtl::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    [encoder setVertexBuffer:cinder::gfx::mtl::bridge<id<MTLBuffer>>(handle_) offset:0 atIndex:1];
}

void GpuBuffer::bindIndex(Commands cmd) const { unwrap(cmd).indexBuffer = handle_; }

GpuBuffer::~GpuBuffer() { release(); }

void GpuBuffer::release() {
    cinder::gfx::mtl::release(handle_);
    mapped_ = nullptr;
}

}
