#include "Gfx/VK/FrameSync.hpp"

#include "Gfx/VK/Ctx.hpp"
#include "Gfx/VK/VkUtil.hpp"

#include <limits>

namespace cinder::Gfx::VK {

FrameSync::FrameSync(const RHI::Ctx& ctx, uint32_t framesInFlight, uint32_t imageCount)
    : ctx_(ctx), framesInFlight_(framesInFlight) {
    VkSemaphoreCreateInfo semaphore{};
    semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence{};
    fence.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    imageAvailable_.resize(framesInFlight);
    inFlight_.resize(framesInFlight);

    for (uint32_t i = 0; i < framesInFlight; ++i) {
        check(vkCreateSemaphore(ctx.device(), &semaphore, nullptr, &imageAvailable_[i]),
              "vkCreateSemaphore");
        check(vkCreateFence(ctx.device(), &fence, nullptr, &inFlight_[i]), "vkCreateFence");
    }

    createPerImage(imageCount);
}

void FrameSync::createPerImage(uint32_t imageCount) {
    VkSemaphoreCreateInfo semaphore{};
    semaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    renderFinished_.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i) {
        check(vkCreateSemaphore(ctx_.device(), &semaphore, nullptr, &renderFinished_[i]),
              "vkCreateSemaphore");
    }
    imagesInFlight_.assign(imageCount, VK_NULL_HANDLE);
}

void FrameSync::destroyPerImage() {
    for (VkSemaphore handle : renderFinished_) vkDestroySemaphore(ctx_.device(), handle, nullptr);
    renderFinished_.clear();
    imagesInFlight_.clear();
}

void FrameSync::resize(uint32_t imageCount) {
    if (imageCount == renderFinished_.size()) {
        releaseImages();
        return;
    }
    destroyPerImage();
    createPerImage(imageCount);
}

void FrameSync::waitForFrame() const {
    check(vkWaitForFences(ctx_.device(), 1, &inFlight_[frame_], VK_TRUE, std::numeric_limits<uint64_t>::max()),
          "vkWaitForFences");
}

void FrameSync::claimImage(uint32_t imageIndex) {
    if (imagesInFlight_[imageIndex] != VK_NULL_HANDLE) {
        check(vkWaitForFences(ctx_.device(), 1, &imagesInFlight_[imageIndex], VK_TRUE,
                              std::numeric_limits<uint64_t>::max()),
              "vkWaitForFences");
    }
    imagesInFlight_[imageIndex] = inFlight_[frame_];
    check(vkResetFences(ctx_.device(), 1, &inFlight_[frame_]), "vkResetFences");
}

void FrameSync::advance() { frame_ = (frame_ + 1) % framesInFlight_; }

void FrameSync::releaseImages() {
    imagesInFlight_.assign(imagesInFlight_.size(), VK_NULL_HANDLE);
}

FrameSync::~FrameSync() {
    destroyPerImage();
    for (VkSemaphore handle : imageAvailable_) vkDestroySemaphore(ctx_.device(), handle, nullptr);
    for (VkFence handle : inFlight_) vkDestroyFence(ctx_.device(), handle, nullptr);
}

}
