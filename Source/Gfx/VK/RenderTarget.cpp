#include "Gfx/VK/RenderTarget.hpp"

#include "Gfx/VK/Formats.hpp"
#include "Gfx/VK/Ctx.hpp"
#include "Gfx/VK/VkRenderPasses.hpp"

namespace cinder::Gfx::RHI {

using cinder::Gfx::VK::DepthBuffer;
using cinder::Gfx::RHI::Ctx;
namespace images = cinder::Gfx::VK::images;
namespace renderPasses = cinder::Gfx::VK::renderPasses;

RenderTarget::RenderTarget(const RHI::Ctx& ctx, VkRenderPass scenePass, VkRenderPass resolvePass,
                           Format sceneFormat, Format resolvedFormat, uint32_t width, uint32_t height)
    : ctx_(ctx), width_(width), height_(height) {
    const VkFormat color = toVk(sceneFormat);
    image_ = images::create(ctx, color, width, height,
                            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    view_ = images::view(ctx, image_.image, color, VK_IMAGE_ASPECT_COLOR_BIT);

    depth_ = std::make_unique<DepthBuffer>(ctx, toVk(ctx.depthFormat()),
                                          width, height);

    const VkImageView attachments[] = {view_, depth_->view()};
    framebuffer_ = images::framebuffer(ctx, scenePass, attachments, 2, width, height);

    const VkRenderPass overlayPass = renderPasses::overlay(ctx, sceneFormat);
    overlayFramebuffer_ = images::framebuffer(ctx, overlayPass, &view_, 1, width, height);
    vkDestroyRenderPass(ctx.device(), overlayPass, nullptr);

    const VkFormat resolved = toVk(resolvedFormat);
    resolved_ = images::create(ctx, resolved, width, height,
                               VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
                                       | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    resolvedView_ = images::view(ctx, resolved_.image, resolved, VK_IMAGE_ASPECT_COLOR_BIT);
    resolveFramebuffer_ = images::framebuffer(ctx, resolvePass, &resolvedView_, 1, width, height);

    pool_ = std::make_unique<TexturePool>(ctx, 1, SamplerFilter::Linear);
    binding_ = pool_->bind(view_);
    depthPool_ = std::make_unique<TexturePool>(ctx, 1, SamplerFilter::Nearest);
    depthBinding_ = depthPool_->bind(depth_->view());
    resolvedPool_ = std::make_unique<TexturePool>(ctx, 1, SamplerFilter::Linear);
    resolvedBinding_ = resolvedPool_->bind(resolvedView_);
}

RenderTarget::~RenderTarget() {
    resolvedPool_.reset();
    depthPool_.reset();
    pool_.reset();
    vkDestroyFramebuffer(ctx_.device(), resolveFramebuffer_, nullptr);
    vkDestroyImageView(ctx_.device(), resolvedView_, nullptr);
    vmaDestroyImage(ctx_.allocator(), resolved_.image, resolved_.allocation);
    vkDestroyFramebuffer(ctx_.device(), overlayFramebuffer_, nullptr);
    vkDestroyFramebuffer(ctx_.device(), framebuffer_, nullptr);
    depth_.reset();
    vkDestroyImageView(ctx_.device(), view_, nullptr);
    vmaDestroyImage(ctx_.allocator(), image_.image, image_.allocation);
}

}
