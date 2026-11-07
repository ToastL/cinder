#pragma once

#include "Gfx/RHI/Format.hpp"
#include "Gfx/RHI/Fwd.hpp"
#include "Gfx/RHI/Handles.hpp"

#include <glm/vec2.hpp>

#include <volk.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace cinder::Platform { class Window; }

namespace cinder::Gfx::RHI { class Ctx; }

namespace cinder::Gfx::VK {
class Swapchain;
class FrameSync;
}

namespace cinder::Gfx::RHI {

struct Frame {
    std::uint32_t index = 0;
    std::uint32_t image = 0;
    Commands commands;
    Uploads uploads;
};

class Presenter {
public:
    Presenter(const cinder::Gfx::RHI::Ctx& ctx, cinder::Platform::Window& window,
              std::uint32_t framesInFlight);
    ~Presenter();

    Presenter(const Presenter&) = delete;
    Presenter& operator=(const Presenter&) = delete;

    void onRecreated(std::function<void(bool formatChanged)> handler) {
        recreated_ = std::move(handler);
    }

    const cinder::Gfx::RHI::Ctx& ctx() const { return ctx_; }
    Format colorFormat() const;
    VkRenderPass renderPass(PassKind pass) const;
    glm::uvec2 extent() const;
    float pixelsPerPoint() const { return pixelsPerPoint_; }

    std::unique_ptr<RenderTarget> createTarget(glm::uvec2 size) const;

    std::optional<Frame> begin();
    void beginScenePass(Frame& frame, const RenderTarget& target, float r, float g, float b);
    void beginPresentPass(Frame& frame, float r, float g, float b);
    void endPass(Frame& frame);
    void end(Frame& frame);

    void captureTarget(const RenderTarget& target, const std::string& path) const;
    void requestWindowCapture(const std::string& path);

private:
    void createRenderPasses();
    void destroyRenderPasses();
    void createCommandBuffers();
    void measureScale();
    void recreateSwapchain();
    void recordWindowCapture(VkCommandBuffer cmd, std::uint32_t imageIndex);

    const cinder::Gfx::RHI::Ctx& ctx_;
    cinder::Platform::Window& window_;
    std::uint32_t framesInFlight_ = 0;

    std::unique_ptr<cinder::Gfx::VK::Swapchain> swapchain_;
    std::unique_ptr<cinder::Gfx::VK::FrameSync> sync_;
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
