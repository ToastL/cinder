#pragma once

#include "gfx/rhi/Format.hpp"
#include "gfx/rhi/Fwd.hpp"
#include "gfx/rhi/Handles.hpp"

#include <glm/vec2.hpp>

#include <volk.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cinder::platform { class Window; }

namespace cinder::gfx::rhi { class Ctx; }

namespace cinder::gfx::vk {
class Swapchain;
class FrameSync;
}

namespace cinder::gfx::rhi {

struct Frame {
    std::uint32_t index = 0;
    std::uint32_t image = 0;
    Commands commands;
    Uploads uploads;
};

class Presenter {
public:
    Presenter(const cinder::gfx::rhi::Ctx& ctx, cinder::platform::Window& window,
              std::uint32_t framesInFlight);
    ~Presenter();

    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    void onRecreated(std::function<void(bool formatChanged)> handler) {
        recreated_ = std::move(handler);
    }

    const cinder::gfx::rhi::Ctx& ctx() const { return ctx_; }
    Format colorFormat() const;
    VkRenderPass renderPass(PassKind pass) const;
    glm::uvec2 extent() const;
    float pixelsPerPoint() const { return pixelsPerPoint_; }

    std::unique_ptr<RenderTarget> createTarget(glm::uvec2 size) const;

    std::optional<Frame> begin();
    void beginScenePass(const Frame& frame, const RenderTarget& target, float r, float g, float b);
    void beginPresentPass(const Frame& frame, float r, float g, float b);
    void endPass(const Frame& frame);
    void end(const Frame& frame);

    void captureTarget(const RenderTarget& target, const std::string& path) const;
    void requestWindowCapture(const std::string& path);
    bool windowCapturePending() const { return !windowCapture_.empty(); }

private:
    void createRenderPasses();
    void destroyRenderPasses();
    void createCommandBuffers();
    void measureScale();
    void recreateSwapchain();
    void recordWindowCapture(VkCommandBuffer cmd, std::uint32_t imageIndex);

    const cinder::gfx::rhi::Ctx& ctx_;
    cinder::platform::Window& window_;
    std::uint32_t framesInFlight_ = 0;

    std::unique_ptr<cinder::gfx::vk::Swapchain> swapchain_;
    std::unique_ptr<cinder::gfx::vk::FrameSync> sync_;
    std::vector<VkCommandBuffer> commandBuffers_;
    VkRenderPass scenePass_ = VK_NULL_HANDLE;
    VkRenderPass presentPass_ = VK_NULL_HANDLE;

    std::function<void(bool)> recreated_;

    std::string windowCapture_;
    std::unique_ptr<GpuBuffer> captureBuffer_;
    VkImageUsageFlags swapchainUsage_ = 0;
    bool captureRecorded_ = false;
    float pixelsPerPoint_ = 1.0f;
};

}
