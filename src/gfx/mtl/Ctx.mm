#include "gfx/mtl/Ctx.hpp"

#include "gfx/mtl/MetalUtil.hpp"
#include "platform/Log.hpp"

namespace cinder::gfx::rhi {

Ctx::Ctx(cinder::platform::Window&) {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) throw cinder::gfx::mtl::error("No Metal-capable GPU found");
    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (queue == nil) throw cinder::gfx::mtl::error("Could not create Metal command queue");

    device_ = cinder::gfx::mtl::retain(device);
    queue_ = cinder::gfx::mtl::retain(queue);
    cinder::platform::logInfo("[mtl] using %s\n", device.name.UTF8String);
}

void Ctx::waitIdle() const {
    id<MTLCommandQueue> queue = cinder::gfx::mtl::bridge<id<MTLCommandQueue>>(queue_);
    id<MTLCommandBuffer> buffer = [queue commandBuffer];
    [buffer commit];
    [buffer waitUntilCompleted];
}

Ctx::~Ctx() {
    waitIdle();
    cinder::gfx::mtl::release(queue_);
    cinder::gfx::mtl::release(device_);
}

}
