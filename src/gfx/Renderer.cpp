#include "gfx/Renderer.hpp"

#include "gfx/RendererDrawList.hpp"
#include "gfx/pass/MeshPass.hpp"
#include "gfx/pass/SpritePass.hpp"
#include "gfx/vk/DepthBuffer.hpp"
#include "gfx/vk/VkCtx.hpp"
#include "gfx/vk/VkRenderPasses.hpp"
#include "gfx/vk/VkUtil.hpp"
#include "platform/Window.hpp"
#include "scene/DrawList.hpp"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <algorithm>
#include <cmath>
#include <cstring>
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

Renderer::Renderer(const VkCtx& ctx, cinder::platform::Window& window,
                   const OverlayFactory& overlay)
    : ctx_(ctx), window_(window) {
    swapchain_ = std::make_unique<Swapchain>(ctx, window);
    depthFormat_ = DepthBuffer::chooseFormat(ctx.physicalDevice());

    sceneRenderPass_ = renderPasses::scene(ctx, swapchain_->format(), depthFormat_);
    presentRenderPass_ = renderPasses::present(ctx, swapchain_->format());

    textureLayout_ = createTextureLayout();
    createTargets();
    swapchain_->createFramebuffers(presentRenderPass_);
    createCommandBuffers();

    sync_ = std::make_unique<FrameSync>(ctx, FRAMES_IN_FLIGHT, swapchain_->imageCount());
    assets_ = std::make_unique<Assets>(ctx, textureLayout_);
    spritePipeline_ = std::make_unique<cinder::gfx::pass::SpritePipeline>(
            ctx, sceneRenderPass_, textureLayout_);
    meshPipeline_ = std::make_unique<cinder::gfx::pass::MeshPipeline>(
            ctx, sceneRenderPass_, textureLayout_);
    compositePipeline_ = std::make_unique<CompositePipeline>(ctx, presentRenderPass_, textureLayout_);

    auto meshPass = std::make_unique<cinder::gfx::pass::MeshPass>(ctx, *assets_, *meshPipeline_);
    auto spritePass = std::make_unique<cinder::gfx::pass::SpritePass>(
            ctx, *assets_, *spritePipeline_, FRAMES_IN_FLIGHT);

    meshPass_ = meshPass.get();
    spritePass_ = spritePass.get();
    draws_ = std::make_unique<RendererDrawList>(*this, *meshPass, *spritePass);

    passes_.push_back(std::move(meshPass));
    passes_.push_back(std::move(spritePass));

    resizePasses();

    if (overlay) {
        overlay_ = overlay(ctx, window, presentRenderPass_, swapchain_->imageCount(),
                           swapchain_->imageCount());
    }
}

VkDescriptorSetLayout Renderer::createTextureLayout() {
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.bindingCount = 1;
    info.pBindings = &binding;

    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    check(vkCreateDescriptorSetLayout(ctx_.device(), &info, nullptr, &layout),
          "vkCreateDescriptorSetLayout");
    return layout;
}

VkExtent2D Renderer::targetExtent() const {
    if (!embedded()) return {swapchain_->width(), swapchain_->height()};

    const float scale = static_cast<float>(window_.width())
            / static_cast<float>(std::max(1, window_.logicalWidth()));
    const auto pixels = [scale](int points) {
        return static_cast<uint32_t>(std::max(1L, std::lround(static_cast<float>(points) * scale)));
    };
    return {pixels(viewportWidth_), pixels(viewportHeight_)};
}

void Renderer::createTargets() {
    destroyTargets();
    const VkExtent2D extent = targetExtent();
    for (uint32_t i = 0; i < FRAMES_IN_FLIGHT; ++i) {
        targets_.push_back(std::make_unique<RenderTarget>(
                ctx_, sceneRenderPass_, textureLayout_, swapchain_->format(), depthFormat_,
                extent.width, extent.height));
        if (embedded() && overlay_ != nullptr) {
            viewportTextures_.push_back(overlay_->addTexture(targets_.back()->view()));
        }
    }
}

void Renderer::destroyTargets() {
    if (overlay_ != nullptr) {
        for (VkDescriptorSet texture : viewportTextures_) overlay_->removeTexture(texture);
    }
    viewportTextures_.clear();
    targets_.clear();
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

void Renderer::resizePasses() {
    const int width = embedded() ? viewportWidth_ : window_.logicalWidth();
    const int height = embedded() ? viewportHeight_ : window_.logicalHeight();
    for (const std::unique_ptr<DrawPass>& pass : passes_) pass->resize(width, height);
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
    resizePasses();
}

void Renderer::overrideCamera3d(const cinder::gfx::pass::PerspectiveCamera& camera) {
    meshPass_->overrideCamera(camera);
}

void Renderer::releaseCamera3d() { meshPass_->releaseCamera(); }

glm::vec3 Renderer::screenToWorld2d(float x, float y) { return spritePass_->screenToWorld(x, y); }

glm::mat4 Renderer::viewProjection2d() { return spritePass_->camera().viewProjection(); }

void Renderer::beginFrame() {
    for (const std::unique_ptr<DrawPass>& pass : passes_) pass->beginFrame();

    if (overlay_ == nullptr) return;
    overlay_->beginFrame();
    if (overlayDraw_) overlayDraw_();
}

VkDescriptorSet Renderer::viewport() const {
    if (viewportTextures_.empty()) return VK_NULL_HANDLE;
    return viewportTextures_[sync_->frame()];
}

void Renderer::registerApi(cinder::lua::LuaApi& api) {
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
    RenderTarget& target = *targets_[sync_->frame()];

    VkCommandBufferBeginInfo begin{};
    begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    check(vkBeginCommandBuffer(cmd, &begin), "vkBeginCommandBuffer");

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
    for (const std::unique_ptr<DrawPass>& pass : passes_) pass->record(cmd, sync_->frame());
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
    if (overlay_ != nullptr) overlay_->record(cmd);
    vkCmdEndRenderPass(cmd);

    check(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");
}

void Renderer::drawFrame() {
    sync_->waitForFrame();

    uint32_t imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(ctx_.device(), swapchain_->handle(),
                                            std::numeric_limits<uint64_t>::max(),
                                            sync_->imageAvailable(), VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        if (overlay_ != nullptr) overlay_->discardFrame();
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
    destroyTargets();
    swapchain_ = std::make_unique<Swapchain>(ctx_, window_);

    if (swapchain_->format() != previousFormat) {
        vkDestroyRenderPass(ctx_.device(), presentRenderPass_, nullptr);
        vkDestroyRenderPass(ctx_.device(), sceneRenderPass_, nullptr);
        sceneRenderPass_ = renderPasses::scene(ctx_, swapchain_->format(), depthFormat_);
        presentRenderPass_ = renderPasses::present(ctx_, swapchain_->format());
    }

    createTargets();
    swapchain_->createFramebuffers(presentRenderPass_);
    resizePasses();
    sync_->resize(swapchain_->imageCount());
    if (overlay_ != nullptr) overlay_->setMinImageCount(swapchain_->imageCount());

    window_.clearResized();
}

void Renderer::capture(const std::string& path) {
    ctx_.waitIdle();

    RenderTarget& target = *targets_[lastFrame_];
    const uint32_t width = target.width();
    const uint32_t height = target.height();
    const VkDeviceSize size = static_cast<VkDeviceSize>(width) * height * 4;

    cinder::gfx::vk::GpuBuffer staging(ctx_, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true);

    VkCommandBuffer cmd = ctx_.beginSingleTime();

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

    ctx_.endSingleTime(cmd);

    std::vector<unsigned char> pixels(static_cast<std::size_t>(size));
    std::memcpy(pixels.data(), staging.mapped(), static_cast<std::size_t>(size));

    const bool bgra = swapchain_->format() == VK_FORMAT_B8G8R8A8_SRGB
            || swapchain_->format() == VK_FORMAT_B8G8R8A8_UNORM;
    if (bgra) {
        for (std::size_t i = 0; i < pixels.size(); i += 4) std::swap(pixels[i], pixels[i + 2]);
    }

    stbi_write_png(path.c_str(), static_cast<int>(width), static_cast<int>(height), 4,
                   pixels.data(), static_cast<int>(width) * 4);
}

cinder::scene::DrawList& Renderer::draws() { return *draws_; }

Renderer::~Renderer() {
    overlay_.reset();
    draws_.reset();
    passes_.clear();
    compositePipeline_.reset();
    meshPipeline_.reset();
    spritePipeline_.reset();
    assets_.reset();
    sync_.reset();

    if (!commandBuffers_.empty()) {
        vkFreeCommandBuffers(ctx_.device(), ctx_.commandPool(),
                             static_cast<uint32_t>(commandBuffers_.size()), commandBuffers_.data());
    }

    swapchain_.reset();
    destroyTargets();

    vkDestroyDescriptorSetLayout(ctx_.device(), textureLayout_, nullptr);
    vkDestroyRenderPass(ctx_.device(), presentRenderPass_, nullptr);
    vkDestroyRenderPass(ctx_.device(), sceneRenderPass_, nullptr);
}

}
