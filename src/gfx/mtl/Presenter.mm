#include "gfx/mtl/Presenter.hpp"

#include "gfx/mtl/Ctx.hpp"
#include "gfx/mtl/MetalUtil.hpp"
#include "gfx/mtl/RenderTarget.hpp"
#include "gfx/rhi/Capture.hpp"
#include "platform/Log.hpp"
#include "platform/Window.hpp"

#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <algorithm>
#include <cstring>
#include <vector>

namespace cinder::gfx::rhi {
namespace {

constexpr NSUInteger COPY_ALIGNMENT = 256;

NSUInteger alignedRowBytes(std::uint32_t width) {
    const NSUInteger bytes = static_cast<NSUInteger>(width) * 4;
    return (bytes + COPY_ALIGNMENT - 1) & ~(COPY_ALIGNMENT - 1);
}

id<MTLBuffer> encodeCapture(id<MTLDevice> device, id<MTLCommandBuffer> commandBuffer,
                            id<MTLTexture> texture, std::uint32_t width,
                            std::uint32_t height) {
    const NSUInteger rowBytes = alignedRowBytes(width);
    id<MTLBuffer> buffer = [device newBufferWithLength:rowBytes * height
                                               options:MTLResourceStorageModeShared];
    if (buffer == nil) throw cinder::gfx::mtl::error("Could not create Metal capture buffer");
    id<MTLBlitCommandEncoder> blit = [commandBuffer blitCommandEncoder];
    [blit copyFromTexture:texture sourceSlice:0 sourceLevel:0
             sourceOrigin:MTLOriginMake(0, 0, 0)
               sourceSize:MTLSizeMake(width, height, 1)
                 toBuffer:buffer destinationOffset:0 destinationBytesPerRow:rowBytes
       destinationBytesPerImage:rowBytes * height];
    [blit endEncoding];
    return buffer;
}

void writeBuffer(const std::string& path, id<MTLBuffer> buffer, std::uint32_t width,
                 std::uint32_t height) {
    const NSUInteger rowBytes = alignedRowBytes(width);
    const std::size_t packedRow = static_cast<std::size_t>(width) * 4;
    std::vector<std::uint8_t> pixels(packedRow * height);
    const auto* source = static_cast<const std::uint8_t*>(buffer.contents);
    for (std::uint32_t row = 0; row < height; ++row) {
        std::memcpy(pixels.data() + static_cast<std::size_t>(row) * packedRow,
                    source + static_cast<std::size_t>(row) * rowBytes, packedRow);
    }
    writeCapture(path, width, height, pixels.data(), Format::BGRA8Srgb);
}

}

Presenter::Presenter(const Ctx& ctx, cinder::platform::Window& window,
                     std::uint32_t framesInFlight)
    : ctx_(ctx), window_(window), framesInFlight_(framesInFlight),
      inFlight_(framesInFlight, nullptr) {
    @autoreleasepool {
        id<MTLDevice> device = cinder::gfx::mtl::bridge<id<MTLDevice>>(ctx.device());
        CAMetalLayer* layer = [CAMetalLayer layer];
        layer.device = device;
        layer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
        layer.framebufferOnly = NO;
        layer.maximumDrawableCount = std::max<std::uint32_t>(2, framesInFlight);
        layer.displaySyncEnabled = YES;

        NSWindow* native = glfwGetCocoaWindow(window.handle());
        NSView* view = native.contentView;
        view.wantsLayer = YES;
        view.layer = layer;
        layer.frame = view.bounds;

        layer_ = cinder::gfx::mtl::retain(layer);
        resize();
    }
}

glm::uvec2 Presenter::extent() const {
    return {static_cast<std::uint32_t>(std::max(window_.width(), 0)),
            static_cast<std::uint32_t>(std::max(window_.height(), 0))};
}

void Presenter::resize() {
    CAMetalLayer* layer = cinder::gfx::mtl::bridge<CAMetalLayer*>(layer_);
    NSWindow* native = glfwGetCocoaWindow(window_.handle());
    layer.frame = native.contentView.bounds;
    const glm::uvec2 size = extent();
    layer.drawableSize = CGSizeMake(size.x, size.y);
    pixelsPerPoint_ = window_.logicalWidth() > 0
            ? static_cast<float>(size.x) / static_cast<float>(window_.logicalWidth())
            : 1.0f;
}

std::unique_ptr<RenderTarget> Presenter::createTarget(glm::uvec2 size) const {
    return std::make_unique<RenderTarget>(ctx_, colorFormat(), size.x, size.y);
}

std::optional<Frame> Presenter::begin() {
    @autoreleasepool {
        if (window_.isMinimized()) return std::nullopt;
        if (window_.wasResized()) {
            ctx_.waitIdle();
            resize();
            if (recreated_) recreated_(false);
            window_.clearResized();
        }

        if (inFlight_[frame_] != nullptr) {
            id<MTLCommandBuffer> previous =
                    cinder::gfx::mtl::bridge<id<MTLCommandBuffer>>(inFlight_[frame_]);
            [previous waitUntilCompleted];
            cinder::gfx::mtl::release(inFlight_[frame_]);
        }

        CAMetalLayer* layer = cinder::gfx::mtl::bridge<CAMetalLayer*>(layer_);
        id<CAMetalDrawable> drawable = [layer nextDrawable];
        if (drawable == nil) return std::nullopt;
        id<MTLCommandQueue> queue = cinder::gfx::mtl::bridge<id<MTLCommandQueue>>(ctx_.queue());
        id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
        if (commandBuffer == nil) throw cinder::gfx::mtl::error("Could not create Metal command buffer");

        Frame frame;
        frame.index = frame_;
        frame.state = std::make_unique<cinder::gfx::mtl::CommandState>();
        frame.commands = cinder::gfx::rhi::commands(frame.state.get());
        frame.uploads = cinder::gfx::rhi::uploads((__bridge void*)commandBuffer);
        frame.commandBuffer = cinder::gfx::mtl::retain(commandBuffer);
        frame.drawable = cinder::gfx::mtl::retain(drawable);
        return frame;
    }
}

void Presenter::beginScenePass(Frame& frame, const RenderTarget& target, float r, float g,
                               float b) {
    id<MTLCommandBuffer> buffer =
            cinder::gfx::mtl::bridge<id<MTLCommandBuffer>>(frame.commandBuffer);
    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture =
            cinder::gfx::mtl::bridge<id<MTLTexture>>(target.color());
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(r, g, b, 1.0);
    pass.depthAttachment.texture = cinder::gfx::mtl::bridge<id<MTLTexture>>(target.depth());
    pass.depthAttachment.loadAction = MTLLoadActionClear;
    pass.depthAttachment.storeAction = MTLStoreActionDontCare;
    pass.depthAttachment.clearDepth = 1.0;
    id<MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:pass];
    if (encoder == nil) throw cinder::gfx::mtl::error("Could not begin Metal scene pass");
    frame.state->encoder = (__bridge void*)encoder;
}

void Presenter::beginPresentPass(Frame& frame, float r, float g, float b) {
    id<MTLCommandBuffer> buffer =
            cinder::gfx::mtl::bridge<id<MTLCommandBuffer>>(frame.commandBuffer);
    id<CAMetalDrawable> drawable =
            cinder::gfx::mtl::bridge<id<CAMetalDrawable>>(frame.drawable);
    MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
    pass.colorAttachments[0].texture = drawable.texture;
    pass.colorAttachments[0].loadAction = MTLLoadActionClear;
    pass.colorAttachments[0].storeAction = MTLStoreActionStore;
    pass.colorAttachments[0].clearColor = MTLClearColorMake(r, g, b, 1.0);
    id<MTLRenderCommandEncoder> encoder = [buffer renderCommandEncoderWithDescriptor:pass];
    if (encoder == nil) throw cinder::gfx::mtl::error("Could not begin Metal present pass");
    frame.state->encoder = (__bridge void*)encoder;
}

void Presenter::endPass(Frame& frame) {
    id<MTLRenderCommandEncoder> encoder =
            cinder::gfx::mtl::bridge<id<MTLRenderCommandEncoder>>(frame.state->encoder);
    [encoder endEncoding];
    frame.state->encoder = nullptr;
}

void Presenter::end(Frame& frame) {
    @autoreleasepool {
        id<MTLCommandBuffer> buffer =
                cinder::gfx::mtl::bridge<id<MTLCommandBuffer>>(frame.commandBuffer);
        id<CAMetalDrawable> drawable =
                cinder::gfx::mtl::bridge<id<CAMetalDrawable>>(frame.drawable);

        id<MTLBuffer> captureBuffer = nil;
        const bool capturing = !windowCapture_.empty();
        if (capturing) {
            const glm::uvec2 size = extent();
            id<MTLDevice> device = cinder::gfx::mtl::bridge<id<MTLDevice>>(ctx_.device());
            captureBuffer = encodeCapture(device, buffer, drawable.texture, size.x, size.y);
        }

        [buffer presentDrawable:drawable];
        [buffer commit];
        if (capturing) {
            [buffer waitUntilCompleted];
            const glm::uvec2 size = extent();
            writeBuffer(windowCapture_, captureBuffer, size.x, size.y);
            cinder::platform::logInfo("[capture] wrote %s\n", windowCapture_.c_str());
            windowCapture_.clear();
        }

        cinder::gfx::mtl::release(frame.drawable);
        inFlight_[frame.index] = frame.commandBuffer;
        frame.commandBuffer = nullptr;
        frame_ = (frame_ + 1) % framesInFlight_;
    }
}

void Presenter::capture(void* textureHandle, std::uint32_t width, std::uint32_t height,
                        const std::string& path) const {
    @autoreleasepool {
        id<MTLDevice> device = cinder::gfx::mtl::bridge<id<MTLDevice>>(ctx_.device());
        id<MTLCommandQueue> queue = cinder::gfx::mtl::bridge<id<MTLCommandQueue>>(ctx_.queue());
        id<MTLCommandBuffer> commandBuffer = [queue commandBuffer];
        id<MTLTexture> texture = cinder::gfx::mtl::bridge<id<MTLTexture>>(textureHandle);
        id<MTLBuffer> buffer = encodeCapture(device, commandBuffer, texture, width, height);
        [commandBuffer commit];
        [commandBuffer waitUntilCompleted];
        writeBuffer(path, buffer, width, height);
    }
}

void Presenter::captureTarget(const RenderTarget& target, const std::string& path) const {
    capture(target.color(), target.width(), target.height(), path);
}

Presenter::~Presenter() {
    ctx_.waitIdle();
    for (void*& buffer : inFlight_) cinder::gfx::mtl::release(buffer);
    cinder::gfx::mtl::release(layer_);
}

}
