#pragma once

#include <volk.h>

#include <vector>

namespace cinder::gfx::rhi { class Ctx; }

namespace cinder::gfx::vk {

class FrameSync {
public:
    FrameSync(const rhi::Ctx& ctx, uint32_t framesInFlight, uint32_t imageCount);
    ~FrameSync();

    FrameSync(const FrameSync&) = delete;
    FrameSync& operator=(const FrameSync&) = delete;

    uint32_t frame() const { return frame_; }
    VkSemaphore imageAvailable() const { return imageAvailable_[frame_]; }
    VkFence fence() const { return inFlight_[frame_]; }
    VkSemaphore renderFinished(uint32_t imageIndex) const { return renderFinished_[imageIndex]; }

    void waitForFrame() const;
    void claimImage(uint32_t imageIndex);
    void advance();
    void releaseImages();
    void resize(uint32_t imageCount);

private:
    void createPerImage(uint32_t imageCount);
    void destroyPerImage();

    const rhi::Ctx& ctx_;
    std::vector<VkSemaphore> imageAvailable_;
    std::vector<VkFence> inFlight_;
    std::vector<VkSemaphore> renderFinished_;
    std::vector<VkFence> imagesInFlight_;
    uint32_t framesInFlight_ = 0;
    uint32_t frame_ = 0;
};

}
