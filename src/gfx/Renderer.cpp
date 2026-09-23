#include "gfx/Renderer.hpp"

#include "gfx/RendererDrawList.hpp"
#include "gfx/Capture.hpp"
#include "gfx/pass/MeshPass.hpp"
#include "gfx/pass/SpritePass.hpp"
#include "gfx/vk/DepthBuffer.hpp"
#include "gfx/vk/VkCtx.hpp"
#include "gfx/vk/VkRenderPasses.hpp"
#include "gfx/vk/VkUtil.hpp"
#include "lua/LuaApi.hpp"
#include "platform/Log.hpp"
#include "platform/Window.hpp"
#include "scene/DrawList.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace cinder::gfx {

using cinder::gfx::asset::Assets;
using cinder::gfx::pass::DrawPass;
using cinder::gfx::vk::DepthBuffer;
using cinder::gfx::vk::FrameSync;
using cinder::gfx::vk::Swapchain;
using cinder::gfx::vk::VkCtx;
using cinder::gfx::vk::check;
namespace renderPasses = cinder::gfx::vk::renderPasses;

namespace {

Renderer& self(lua_State* state) { return *cinder::lua::LuaApi::context<Renderer>(state); }

int screenSize(lua_State* state) {
    const glm::vec2 size = self(state).camera().size();
    lua_pushnumber(state, size.x);
    lua_pushnumber(state, size.y);
    return 2;
}

int screenToWorld(lua_State* state) {
    const glm::vec3 world = self(state).camera().screenToWorld(static_cast<float>(lua_tonumber(state, 1)),
                                                              static_cast<float>(lua_tonumber(state, 2)));
    lua_pushnumber(state, world.x);
    lua_pushnumber(state, world.y);
    return 2;
}

}

Renderer::Renderer(const VkCtx& ctx, cinder::platform::Window& window) : ctx_(ctx), window_(window) {
    swapchain_ = std::make_unique<Swapchain>(ctx, window);
    measureScale();
    depthFormat_ = DepthBuffer::chooseFormat(ctx.physicalDevice());

    sceneRenderPass_ = renderPasses::scene(ctx, swapchain_->format(), depthFormat_);
    presentRenderPass_ = renderPasses::present(ctx, swapchain_->format());

    createTargets();
    swapchain_->createFramebuffers(presentRenderPass_);
    createCommandBuffers();

    sync_ = std::make_unique<FrameSync>(ctx, FRAMES_IN_FLIGHT, swapchain_->imageCount());
    assets_ = std::make_unique<Assets>(ctx);
    spritePipeline_ = std::make_unique<cinder::gfx::pass::SpritePipeline>(ctx, sceneRenderPass_);
    meshPipeline_ = std::make_unique<cinder::gfx::pass::MeshPipeline>(ctx, sceneRenderPass_);
    compositePipeline_ = std::make_unique<CompositePipeline>(ctx, presentRenderPass_);
    ui_ = std::make_unique<UiRenderer>(ctx, *assets_, presentRenderPass_, swapchain_->format(),
                                       FRAMES_IN_FLIGHT);

    auto meshPass = std::make_unique<cinder::gfx::pass::MeshPass>(ctx, *assets_, *meshPipeline_);
    auto spritePass = std::make_unique<cinder::gfx::pass::SpritePass>(
            ctx, *assets_, *spritePipeline_, FRAMES_IN_FLIGHT);

    draws_ = std::make_unique<RendererDrawList>(*this, *meshPass, *spritePass);

    passes_.push_back(std::move(meshPass));
    passes_.push_back(std::move(spritePass));

    resizeCameras();
}

void Renderer::measureScale() {
    if (window_.logicalWidth() <= 0) return;
    pixelsPerPoint_ = static_cast<float>(swapchain_->width()) / static_cast<float>(window_.logicalWidth());
}

VkExtent2D Renderer::targetExtent() const {
    if (!embedded()) return {swapchain_->width(), swapchain_->height()};

    const float scale = pixelsPerPoint();
    const auto pixels = [scale](int points) {
        return static_cast<uint32_t>(std::max(1L, std::lround(static_cast<float>(points) * scale)));
    };
    return {pixels(viewportWidth_), pixels(viewportHeight_)};
}

void Renderer::createTargets() {
    targets_.recreate(ctx_, sceneRenderPass_, swapchain_->format(), depthFormat_,
                      targetExtent(), FRAMES_IN_FLIGHT);
}

void Renderer::createCommandBuffers() {
    commandBuffers_.resize(FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.commandPool = ctx_.commandPool();
    info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    info.commandBufferCount = FRAMES_IN_FLIGHT;

    check(vkAllocateCommandBuffers(ctx_.device(), &info, commandBuffers_.data()),
          "vkAllocateCommandBuffers");
}

glm::vec2 Renderer::viewSize() const {
    if (embedded()) return glm::vec2(viewportWidth_, viewportHeight_);
    return glm::vec2(window_.logicalWidth(), window_.logicalHeight());
}

void Renderer::resizeCameras() {
    const glm::vec2 size = viewSize();
    camera_.setViewSize(size.x, size.y);
    if (override_) override_->setViewSize(size.x, size.y);
}

void Renderer::setClearColor(float r, float g, float b) {
    clearR_ = r;
    clearG_ = g;
    clearB_ = b;
}

void Renderer::setViewportSize(int width, int height) {
    if (width == viewportWidth_ && height == viewportHeight_) return;

    ctx_.waitIdle();
    viewportWidth_ = width;
    viewportHeight_ = height;
    createTargets();
    resizeCameras();
}

void Renderer::overrideCamera(const cinder::scene::View& view) {
    if (!override_) override_.emplace(camera_);
    override_->setView(view);
}

void Renderer::releaseCamera() { override_.reset(); }

void Renderer::beginFrame() {
    for (const std::unique_ptr<DrawPass>& pass : passes_) pass->beginFrame();

    uiElements_.reset(glm::vec2(window_.logicalWidth(), window_.logicalHeight()), pixelsPerPoint());
    if (uiPaint_) uiPaint_(uiElements_);
}

void Renderer::registerApi(cinder::lua::LuaApi& api) {
    api.bind("screenSize", screenSize, this);
    api.bind("screenToWorld", screenToWorld, this);
    for (const std::unique_ptr<DrawPass>& pass : passes_) pass->registerApi(api);
}

void Renderer::setViewport(VkCommandBuffer cmd, uint32_t width, uint32_t height) {
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(width);
    viewport.height = static_cast<float>(height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {width, height};
    vkCmdSetScissor(cmd, 0, 1, &scissor);
}

void Renderer::recordCommandBuffer(VkCommandBuffer cmd, uint32_t imageIndex) {
    RenderTarget& target = targets_.at(sync_->frame());

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    check(vkBeginCommandBuffer(cmd, &begin), "vkBeginCommandBuffer");
    ui_->prepare(cmd, sync_->frame(), uiElements_);

    VkClearValue sceneClear[2]{};
    sceneClear[0].color = {{clearR_, clearG_, clearB_, 1.0f}};
    sceneClear[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo scene{};
    scene.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    scene.renderPass = sceneRenderPass_;
    scene.framebuffer = target.framebuffer();
    scene.renderArea.offset = {0, 0};
    scene.renderArea.extent = {target.width(), target.height()};
    scene.clearValueCount = 2;
    scene.pClearValues = sceneClear;

    vkCmdBeginRenderPass(cmd, &scene, VK_SUBPASS_CONTENTS_INLINE);
    setViewport(cmd, target.width(), target.height());
    const glm::mat4& viewProjection = override_ ? override_->viewProjection() : camera_.viewProjection();
    for (const std::unique_ptr<DrawPass>& pass : passes_) pass->record(cmd, sync_->frame(), viewProjection);
    vkCmdEndRenderPass(cmd);

    VkClearValue presentClear{};
    presentClear.color = {{clearR_, clearG_, clearB_, 1.0f}};

    VkRenderPassBeginInfo present{};
    present.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    present.renderPass = presentRenderPass_;
    present.framebuffer = swapchain_->framebuffer(imageIndex);
    present.renderArea.offset = {0, 0};
    present.renderArea.extent = {swapchain_->width(), swapchain_->height()};
    present.clearValueCount = 1;
    present.pClearValues = &presentClear;

    vkCmdBeginRenderPass(cmd, &present, VK_SUBPASS_CONTENTS_INLINE);
    setViewport(cmd, swapchain_->width(), swapchain_->height());
    if (!embedded()) compositePipeline_->draw(cmd, target.descriptorSet());
    ui_->record(cmd, sync_->frame(), {swapchain_->width(), swapchain_->height()}, target.descriptorSet());
    vkCmdEndRenderPass(cmd);
    recordWindowCapture(cmd, imageIndex);

    check(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");
}

void Renderer::drawFrame() {
    sync_->waitForFrame();

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(ctx_.device(), swapchain_->handle(),
                                            std::numeric_limits<uint64_t>::max(),
                                            sync_->imageAvailable(), VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapchain();
        return;
    }
    if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        check(result, "vkAcquireNextImageKHR");
    }

    sync_->claimImage(imageIndex);

    lastFrame_ = sync_->frame();

    VkCommandBuffer cmd = commandBuffers_[sync_->frame()];
    vkResetCommandBuffer(cmd, 0);
    recordCommandBuffer(cmd, imageIndex);

    const VkSemaphore waitSemaphore = sync_->imageAvailable();
    const VkSemaphore signalSemaphore = sync_->renderFinished(imageIndex);
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

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &signalSemaphore;
    present.swapchainCount = 1;
    present.pSwapchains = &handle;
    present.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(ctx_.presentQueue(), &present);
    if (captureRecorded_) {
        ctx_.waitIdle();
        writeCapture(windowCapture_, swapchain_->width(), swapchain_->height(), captureBuffer_->mapped(),
                     swapchain_->format());
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

void Renderer::recreateSwapchain() {
    if (window_.isMinimized()) return;

    ctx_.waitIdle();

    const VkFormat previousFormat = swapchain_->format();

    swapchain_.reset();
    targets_.clear();
    swapchain_ = std::make_unique<Swapchain>(ctx_, window_, swapchainUsage_);
    measureScale();

    if (swapchain_->format() != previousFormat) {
        vkDestroyRenderPass(ctx_.device(), presentRenderPass_, nullptr);
        vkDestroyRenderPass(ctx_.device(), sceneRenderPass_, nullptr);
        sceneRenderPass_ = renderPasses::scene(ctx_, swapchain_->format(), depthFormat_);
        presentRenderPass_ = renderPasses::present(ctx_, swapchain_->format());
        ui_->rebuild(presentRenderPass_, swapchain_->format());
    }

    createTargets();
    swapchain_->createFramebuffers(presentRenderPass_);
    resizeCameras();
    sync_->resize(swapchain_->imageCount());

    window_.clearResized();
}

void Renderer::capture(const std::string& path) {
    captureTarget(ctx_, targets_.at(lastFrame_), swapchain_->format(), path);
}

void Renderer::requestWindowCapture(const std::string& path) {
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

void Renderer::recordWindowCapture(VkCommandBuffer cmd, uint32_t imageIndex) {
    if (windowCapture_.empty() || (swapchainUsage_ & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) == 0) return;

    const uint32_t width = swapchain_->width();
    const uint32_t height = swapchain_->height();
    const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;
    if (!captureBuffer_ || captureBuffer_->size() < size) {
        captureBuffer_ = std::make_unique<cinder::gfx::vk::GpuBuffer>(ctx_, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                                      true);
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
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.imageExtent = {width, height, 1};
    vkCmdCopyImageToBuffer(cmd, barrier.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, captureBuffer_->handle(), 1,
                           &copy);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = 0;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);
    captureRecorded_ = true;
}

cinder::scene::DrawList& Renderer::draws() { return *draws_; }

Renderer::~Renderer() {
    ctx_.waitIdle();
    targets_.clear();
    draws_.reset();
    passes_.clear();
    compositePipeline_.reset();
    captureBuffer_.reset();
    ui_.reset();
    meshPipeline_.reset();
    spritePipeline_.reset();
    assets_.reset();
    sync_.reset();

    if (!commandBuffers_.empty()) {
        vkFreeCommandBuffers(ctx_.device(), ctx_.commandPool(),
                             static_cast<uint32_t>(commandBuffers_.size()), commandBuffers_.data());
    }

    swapchain_.reset();

    vkDestroyRenderPass(ctx_.device(), presentRenderPass_, nullptr);
    vkDestroyRenderPass(ctx_.device(), sceneRenderPass_, nullptr);
}

}
