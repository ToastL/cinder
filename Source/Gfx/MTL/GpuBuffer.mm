#include "Gfx/MTL/GpuBuffer.hpp"

#include "Gfx/MTL/Commands.hpp"
#include "Gfx/MTL/Ctx.hpp"
#include "Gfx/MTL/MetalUtil.hpp"

#include <utility>

namespace cinder::Gfx::RHI {

GpuBuffer::GpuBuffer(const Ctx& ctx, std::uint64_t size, BufferUsage)
    : size_(size) {
    id<MTLDevice> device = cinder::Gfx::MTL::bridge<id<MTLDevice>>(ctx.device());
    id<MTLBuffer> buffer = [device newBufferWithLength:static_cast<NSUInteger>(size)
                                             options:MTLResourceStorageModeShared];
    if (buffer == nil) throw cinder::Gfx::MTL::error("Could not create Metal buffer");
    handle_ = cinder::Gfx::MTL::retain(buffer);
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
            cinder::Gfx::MTL::bridge<id<MTLRenderCommandEncoder>>(state.encoder);
    [encoder setVertexBuffer:cinder::Gfx::MTL::bridge<id<MTLBuffer>>(handle_)
                      offset:0
                     atIndex:cinder::Gfx::MTL::VERTEX_BUFFER_SLOT];
}

void GpuBuffer::bindIndex(Commands cmd) const { unwrap(cmd).indexBuffer = handle_; }

GpuBuffer::~GpuBuffer() { release(); }

void GpuBuffer::release() {
    cinder::Gfx::MTL::release(handle_);
    mapped_ = nullptr;
}

}
