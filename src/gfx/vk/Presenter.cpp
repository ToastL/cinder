#include "gfx/vk/Presenter.hpp"

#include "gfx/rhi/Capture.hpp"
#include "gfx/vk/Commands.hpp"
#include "gfx/vk/FrameSync.hpp"
#include "gfx/vk/GpuBuffer.hpp"
#include "gfx/vk/RenderTarget.hpp"
#include "gfx/vk/Swapchain.hpp"
#include "gfx/vk/Ctx.hpp"
#include "gfx/vk/VkRenderPasses.hpp"
#include "gfx/vk/VkUtil.hpp"
#include "platform/Log.hpp"
#include "platform/Window.hpp"

#include <limits>

namespace cinder::gfx::rhi {

using cinder::gfx::vk::check;
using cinder::gfx::vk::FrameSync;
using cinder::gfx::vk::Swapchain;
namespace renderPasses = cinder::gfx::vk::renderPasses;

Presenter::Presenter(const cinder::gfx::rhi::Ctx& ctx, cinder::platform::Window& window,
                     std::uint32_t framesInFlight)
    : ctx_(ctx), window_(window), framesInFlight_(framesInFlight) {
    swapchain_ = std::make_unique<Swapchain>(ctx, window);
    measureScale();
    createRenderPasses();
    swapchain_->createFramebuffers(presentPass_);
    createCommandBuffers();
    sync_ = std::make_unique<FrameSync>(ctx, framesInFlight, swapchain_->imageCount());
}

void Presenter::createRenderPasses() {
    scenePass_ = renderPasses::scene(ctx_, swapchain_->format());
    presentPass_ = renderPasses::present(ctx_, swapchain_->format());
}

void Presenter::destroyRenderPasses() {
    if (presentPass_ != VK_NULL_HANDLE) vkDestroyRenderPass(ctx_.device(), presentPass_, nullptr);
    if (scenePass_ != VK_NULL_HANDLE) vkDestroyRenderPass(ctx_.device(), scenePass_, nullptr);
    presentPass_ = VK_NULL_HANDLE;
    scenePass_ = VK_NULL_HANDLE;
}

void Presenter::createCommandBuffers() {
    commandBuffers_.resize(framesInFlight_);

    VkCommandBufferAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = ctx_.commandPool();
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = framesInFlight_;

    check(vkAllocateCommandBuffers(ctx_.device(), &info, commandBuffers_.data()),
          "vkAllocateCommandBuffers");
}

void Presenter::measureScale() {
    if (window_.logicalWidth() <= 0) return;
    pixelsPerPoint_ =
            static_cast<float>(swapchain_->width()) / static_cast<float>(window_.logicalWidth());
}

Format Presenter::colorFormat() const { return swapchain_->format(); }

VkRenderPass Presenter::renderPass(PassKind pass) const {
    return pass == PassKind::Scene ? scenePass_ : presentPass_;
}

glm::uvec2 Presenter::extent() const { return {swapchain_->width(), swapchain_->height()}; }

std::unique_ptr<RenderTarget> Presenter::createTarget(glm::uvec2 size) const {
    return std::make_unique<RenderTarget>(ctx_, scenePass_, swapchain_->format(), size.x, size.y);
}

std::optional<Frame> Presenter::begin() {
    sync_->waitForFrame();

    std::uint32_t imageIndex = 0;
    const VkResult result = vkAcquireNextImageKHR(ctx_.device(), swapchain_->handle(),
                                                  std::numeric_limits<std::uint64_t>::max(),
                                                  sync_->imageAvailable(), VK_NULL_HANDLE,
                                                  &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return std::nullopt;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        check(result, "vkAcquireNextImageKHR");
    }

    sync_->claimImage(imageIndex);

    VkCommandBuffer cmd = commandBuffers_[sync_->frame()];
    vkResetCommandBuffer(cmd, 0);

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    check(vkBeginCommandBuffer(cmd, &begin), "vkBeginCommandBuffer");

    Frame frame;
    frame.index = sync_->frame();
    frame.image = imageIndex;
    frame.commands = commands(cmd);
    frame.uploads = uploads(cmd);
    return frame;
}

void Presenter::beginScenePass(Frame& frame, const RenderTarget& target, float r, float g,
                               float b) {
    VkClearValue clears[2]{};
    clears[0].color = {{r, g, b, 1.0f}};
    clears[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = scenePass_;
    info.framebuffer = target.framebuffer();
    info.renderArea.offset = {0, 0};
    info.renderArea.extent = {target.width(), target.height()};
    info.clearValueCount = 2;
    info.pClearValues = clears;

    vkCmdBeginRenderPass(unwrap(frame.commands), &info, VK_SUBPASS_CONTENTS_INLINE);
}

void Presenter::beginPresentPass(Frame& frame, float r, float g, float b) {
    VkClearValue clear{};
    clear.color = {{r, g, b, 1.0f}};

    VkRenderPassBeginInfo info{};
    info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    info.renderPass = presentPass_;
    info.framebuffer = swapchain_->framebuffer(frame.image);
    info.renderArea.offset = {0, 0};
    info.renderArea.extent = {swapchain_->width(), swapchain_->height()};
    info.clearValueCount = 1;
    info.pClearValues = &clear;

    vkCmdBeginRenderPass(unwrap(frame.commands), &info, VK_SUBPASS_CONTENTS_INLINE);
}

void Presenter::endPass(Frame& frame) { vkCmdEndRenderPass(unwrap(frame.commands)); }

void Presenter::end(Frame& frame) {
    VkCommandBuffer cmd = unwrap(frame.commands);
    recordWindowCapture(cmd, frame.image);
    check(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");

    const VkSemaphore waitSemaphore = sync_->imageAvailable();
    const VkSemaphore signalSemaphore = sync_->renderFinished(frame.image);
    const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &waitSemaphore;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &signalSemaphore;

    check(vkQueueSubmit(ctx_.graphicsQueue(), 1, &submit, sync_->fence()), "vkQueueSubmit");

    const VkSwapchainKHR handle = swapchain_->handle();
    std::uint32_t imageIndex = frame.image;

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &signalSemaphore;
    present.swapchainCount = 1;
    present.pSwapchains = &handle;
    present.pImageIndices = &imageIndex;

    const VkResult result = vkQueuePresentKHR(ctx_.presentQueue(), &present);
    if (captureRecorded_) {
        ctx_.waitIdle();
        writeCapture(windowCapture_, swapchain_->width(), swapchain_->height(),
                                  captureBuffer_->mapped(), swapchain_->format());
        cinder::platform::logInfo("[capture] wrote %s\n", windowCapture_.c_str());
        windowCapture_.clear();
        captureRecorded_ = false;
    }
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || window_.wasResized()) {
        recreateSwapchain();
    } else if (result != VK_SUCCESS) {
        check(result, "vkQueuePresentKHR");
    }

    sync_->advance();
}

void Presenter::recreateSwapchain() {
    if (window_.isMinimized()) return;

    ctx_.waitIdle();

    const Format previousFormat = swapchain_->format();

    swapchain_.reset();
    swapchain_ = std::make_unique<Swapchain>(ctx_, window_, swapchainUsage_);
    measureScale();

    const bool formatChanged = swapchain_->format() != previousFormat;
    if (formatChanged) {
        destroyRenderPasses();
        createRenderPasses();
    }

    if (recreated_) recreated_(formatChanged);

    swapchain_->createFramebuffers(presentPass_);
    sync_->resize(swapchain_->imageCount());

    window_.clearResized();
}

void Presenter::captureTarget(const RenderTarget& target, const std::string& path) const {
    const cinder::gfx::rhi::Ctx& ctx = ctx_;
    const Format format = swapchain_->format();
    ctx.waitIdle();

    const uint32_t width = target.width();
    const uint32_t height = target.height();
    const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;

    GpuBuffer staging(ctx, size, BufferUsage::TransferDst, true);

    VkCommandBuffer cmd = ctx.beginSingleTime();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = target.image();
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.imageExtent = {width, height, 1};
    vkCmdCopyImageToBuffer(cmd, target.image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           staging.handle(), 1, &copy);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr,
                         1, &barrier);

    ctx.endSingleTime(cmd);

    writeCapture(path, width, height, staging.mapped(), format);
}

void Presenter::requestWindowCapture(const std::string& path) {
    if (!Swapchain::supports(ctx_, VK_IMAGE_USAGE_TRANSFER_SRC_BIT)) {
        cinder::platform::logError("[capture] this device cannot copy out of the swapchain\n");
        return;
    }
    windowCapture_ = path;
    if ((swapchainUsage_ & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0) {
        swapchainUsage_ |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        recreateSwapchain();
    }
}

void Presenter::recordWindowCapture(VkCommandBuffer cmd, std::uint32_t imageIndex) {
    if (windowCapture_.empty() || (swapchainUsage_ & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0) return;

    const std::uint32_t width = swapchain_->width();
    const std::uint32_t height = swapchain_->height();
    const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;
    if (!captureBuffer_ || captureBuffer_->size() < size) {
        captureBuffer_ =
                std::make_unique<GpuBuffer>(ctx_, size, BufferUsage::TransferDst, true);
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = swapchain_->image(imageIndex);
    barrier.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.imageExtent = {width, height, 1};
    vkCmdCopyImageToBuffer(cmd, barrier.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           captureBuffer_->handle(), 1, &copy);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = 0;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    captureRecorded_ = true;
}

Presenter::~Presenter() {
    ctx_.waitIdle();
    captureBuffer_.reset();
    sync_.reset();

    if (!commandBuffers_.empty()) {
        vkFreeCommandBuffers(ctx_.device(), ctx_.commandPool(),
                             static_cast<std::uint32_t>(commandBuffers_.size()),
                             commandBuffers_.data());
    }

    destroyRenderPasses();
    swapchain_.reset();
}

}
