#include "Gfx/MTL/Ctx.hpp"

#include "Gfx/MTL/MetalUtil.hpp"
#include "Platform/Log.hpp"

namespace cinder::Gfx::RHI {

Ctx::Ctx(cinder::Platform::Window&) {
    id<MTLDevice> device = MTLCreateSystemDefaultDevice();
    if (device == nil) throw cinder::Gfx::MTL::error("No Metal-capable GPU found");
    id<MTLCommandQueue> queue = [device newCommandQueue];
    if (queue == nil) throw cinder::Gfx::MTL::error("Could not create Metal command queue");

    device_ = cinder::Gfx::MTL::retain(device);
    queue_ = cinder::Gfx::MTL::retain(queue);
    cinder::Platform::logInfo("[mtl] using %s\n", device.name.UTF8String);
}

void Ctx::waitIdle() const {
    const cinder::Gfx::MTL::PoolScope pool;
    id<MTLCommandQueue> queue = cinder::Gfx::MTL::bridge<id<MTLCommandQueue>>(queue_);
    id<MTLCommandBuffer> buffer = [queue commandBuffer];
    [buffer commit];
    [buffer waitUntilCompleted];
}

Ctx::~Ctx() {
    waitIdle();
    cinder::Gfx::MTL::release(queue_);
    cinder::Gfx::MTL::release(device_);
}

}
